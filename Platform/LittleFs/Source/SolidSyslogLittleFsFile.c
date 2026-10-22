/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogLittleFsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogLittleFsFilePrivate.h"
#include "SolidSyslogNullFile.h"
#include "lfs.h"

const struct SolidSyslogErrorSource SolidSyslogLittleFsFileErrorSource = {"LittleFsFile", SolidSyslogFileErrors_Name};

/* The File contract says Open creates the file when absent and never truncates
 * an existing one. */
#define LITTLEFSFILE_READ_WRITE_OR_CREATE (LFS_O_RDWR | LFS_O_CREAT)

static bool LittleFsFile_Open(struct SolidSyslogFile* base, const char* path);
static void LittleFsFile_Close(struct SolidSyslogFile* base);
static bool LittleFsFile_IsOpen(struct SolidSyslogFile* base);
static bool LittleFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool LittleFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void LittleFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static bool LittleFsFile_Size(struct SolidSyslogFile* base, size_t* size);
static void LittleFsFile_Truncate(struct SolidSyslogFile* base);
static bool LittleFsFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool LittleFsFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogLittleFsFile* LittleFsFile_SelfFromBase(struct SolidSyslogFile* base);
static inline bool LittleFsFile_IsValidSetup(const lfs_t* filesystem, const void* fileBuffer, uint32_t fileBufferBytes);
static inline void LittleFsFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeCode);
static inline bool LittleFsFile_IsWholeTransfer(lfs_ssize_t transferred, size_t count);
static inline bool LittleFsFile_Sync(struct SolidSyslogLittleFsFile* self);
static inline int LittleFsFile_SeekToStart(struct SolidSyslogLittleFsFile* self);

bool SolidSyslogLittleFsFile_Initialise(
    struct SolidSyslogFile* base,
    lfs_t* filesystem,
    void* fileBuffer,
    uint32_t fileBufferBytes
)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    bool valid = LittleFsFile_IsValidSetup(filesystem, fileBuffer, fileBufferBytes);
    /* Start from the Null vtable so any slot this adapter has not filled is a
     * safe no-op rather than a NULL dispatch. */
    self->Base = *SolidSyslogNullFile_Get();
    if (valid == true)
    {
        self->Filesystem = filesystem;
        self->OpenConfig.buffer = fileBuffer;
        self->Base.Open = LittleFsFile_Open;
        self->Base.Close = LittleFsFile_Close;
        self->Base.IsOpen = LittleFsFile_IsOpen;
        self->Base.Read = LittleFsFile_Read;
        self->Base.Write = LittleFsFile_Write;
        self->Base.SeekTo = LittleFsFile_SeekTo;
        self->Base.Size = LittleFsFile_Size;
        self->Base.Truncate = LittleFsFile_Truncate;
        self->Base.Exists = LittleFsFile_Exists;
        self->Base.Delete = LittleFsFile_Delete;
        self->IsOpen = false;
    }
    return valid;
}

static inline struct SolidSyslogLittleFsFile* LittleFsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogLittleFsFile*) base;
}

/* Checked here rather than at Open so a wiring fault is one report at Create
 * instead of a surprise on the first record the store writes. The size cannot
 * change later: cache_size is fixed when the integrator mounts. */
static inline bool LittleFsFile_IsValidSetup(const lfs_t* filesystem, const void* fileBuffer, uint32_t fileBufferBytes)
{
    bool valid = false;
    if (filesystem == NULL)
    {
        LittleFsFile_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_FILE_ERROR_NULL_FILESYSTEM
        );
    }
    else if ((fileBuffer == NULL) || (fileBufferBytes < filesystem->cfg->cache_size))
    {
        LittleFsFile_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_FILE_ERROR_BUFFER_TOO_SMALL
        );
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogLittleFsFile_Cleanup(struct SolidSyslogFile* base)
{
    /* Close first: the slot is about to be reusable, and LittleFS would
     * otherwise keep a file open against a buffer the caller may reclaim. */
    LittleFsFile_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static bool LittleFsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    int result =
        lfs_file_opencfg(self->Filesystem, &self->Handle, path, LITTLEFSFILE_READ_WRITE_OR_CREATE, &self->OpenConfig);
    self->IsOpen = (result == LFS_ERR_OK);
    if (!self->IsOpen)
    {
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, result);
    }
    return self->IsOpen;
}

/* The fault, with the negative lfs code behind it when the call answered one.
 * A short transfer answers a count, not a code. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- every caller passes a named code and the result it read
static inline void LittleFsFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeCode)
{
    if (nativeCode != LFS_ERR_OK)
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
            &SolidSyslogLittleFsFileErrorSource,
            SOLIDSYSLOG_CAT_FILE_IO_FAILED,
            (int32_t) code,
            (int32_t) nativeCode
        );
    }
    else
    {
        LittleFsFile_Report(SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY, SOLIDSYSLOG_CAT_FILE_IO_FAILED, code);
    }
}

static void LittleFsFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    if (self->IsOpen)
    {
        int result = lfs_file_close(self->Filesystem, &self->Handle);
        /* The file is released whether or not the close could flush. */
        self->IsOpen = false;
        if (result != LFS_ERR_OK)
        {
            LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, result);
        }
    }
}

static bool LittleFsFile_IsOpen(struct SolidSyslogFile* base)
{
    return LittleFsFile_SelfFromBase(base)->IsOpen;
}

static bool LittleFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    lfs_ssize_t read = lfs_file_read(self->Filesystem, &self->Handle, buf, (lfs_size_t) count);
    /* A short read answers a count, which the store judges; a failure answers
     * a negative code. The contract fails them the same way. */
    if (read < 0)
    {
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, (int) read);
    }
    return LittleFsFile_IsWholeTransfer(read, count);
}

/* lfs_file_read and lfs_file_write answer a negative code, or how many bytes
 * moved. */
static inline bool LittleFsFile_IsWholeTransfer(lfs_ssize_t transferred, size_t count)
{
    return (transferred >= 0) && ((size_t) transferred == count);
}

static bool LittleFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    lfs_ssize_t written = lfs_file_write(self->Filesystem, &self->Handle, buf, (lfs_size_t) count);
    bool wroteAllData = LittleFsFile_IsWholeTransfer(written, count);
    if (wroteAllData)
    {
        /* The File contract makes a true return mean the bytes are on the media:
         * the BlockStore treats it as durable across power loss, so the sync is
         * part of the write rather than something a caller arranges. */
        wroteAllData = LittleFsFile_Sync(self);
    }
    else
    {
        /* A short write has no code behind it. */
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, (written < 0) ? (int) written : LFS_ERR_OK);
    }
    return wroteAllData;
}

static inline bool LittleFsFile_Sync(struct SolidSyslogLittleFsFile* self)
{
    int result = lfs_file_sync(self->Filesystem, &self->Handle);
    if (result != LFS_ERR_OK)
    {
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SYNC_FAILED, result);
    }
    return result == LFS_ERR_OK;
}

static void LittleFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    lfs_soff_t position = lfs_file_seek(self->Filesystem, &self->Handle, (lfs_soff_t) offset, LFS_SEEK_SET);
    /* lfs answers the new position, or a negative code. */
    if (position < 0)
    {
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, (int) position);
    }
}

static bool LittleFsFile_Size(struct SolidSyslogFile* base, size_t* size)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    lfs_soff_t length = lfs_file_size(self->Filesystem, &self->Handle);
    /* lfs reports an error as a negative value. */
    bool known = length >= 0;
    if (!known)
    {
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, (int) length);
    }
    *size = known ? (size_t) length : 0U;
    return known;
}

/* Seek as well as truncate: the contract leaves the file open at length zero,
 * and the shared position would otherwise sit past the new end. The seek is
 * part of the truncate, so either step failing is the truncate failing. */
static void LittleFsFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    int result = lfs_file_truncate(self->Filesystem, &self->Handle, 0);
    if (result == LFS_ERR_OK)
    {
        result = LittleFsFile_SeekToStart(self);
    }
    if (result != LFS_ERR_OK)
    {
        LittleFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, result);
    }
}

/* LFS_ERR_OK, or the negative code the seek answered. */
static inline int LittleFsFile_SeekToStart(struct SolidSyslogLittleFsFile* self)
{
    lfs_soff_t position = lfs_file_seek(self->Filesystem, &self->Handle, 0, LFS_SEEK_SET);
    return (position < 0) ? (int) position : LFS_ERR_OK;
}

/* A path that is not there is the answer, not a fault, so nothing is reported. */
static bool LittleFsFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    struct lfs_info info;
    return lfs_stat(self->Filesystem, path, &info) == LFS_ERR_OK;
}

static bool LittleFsFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    int result = lfs_remove(self->Filesystem, path);
    /* The contract counts an absent path as removed. */
    return (result == LFS_ERR_OK) || (result == LFS_ERR_NOENT);
}
