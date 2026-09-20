/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogLittleFsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogLittleFsFilePrivate.h"
#include "SolidSyslogNullFile.h"
#include "lfs.h"

const struct SolidSyslogErrorSource SolidSyslogLittleFsFileErrorSource = {"LittleFsFile"};

/* The File contract says Open creates the file when absent and never truncates
 * an existing one. */
#define READ_WRITE_OR_CREATE (LFS_O_RDWR | LFS_O_CREAT)

static bool LittleFsFile_Open(struct SolidSyslogFile* base, const char* path);
static void LittleFsFile_Close(struct SolidSyslogFile* base);
static bool LittleFsFile_IsOpen(struct SolidSyslogFile* base);
static bool LittleFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool LittleFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void LittleFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static size_t LittleFsFile_Size(struct SolidSyslogFile* base);
static void LittleFsFile_Truncate(struct SolidSyslogFile* base);
static bool LittleFsFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool LittleFsFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogLittleFsFile* LittleFsFile_SelfFromBase(struct SolidSyslogFile* base);
static inline bool LittleFsFile_IsValidSetup(const lfs_t* filesystem, const void* fileBuffer, uint32_t fileBufferBytes);

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
    if (valid == false)
    {
        return false;
    }
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
    return true;
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
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static bool LittleFsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    int result = lfs_file_opencfg(self->Filesystem, &self->Handle, path, READ_WRITE_OR_CREATE, &self->OpenConfig);
    self->IsOpen = (result == LFS_ERR_OK);
    return self->IsOpen;
}

static void LittleFsFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    if (self->IsOpen)
    {
        (void) lfs_file_close(self->Filesystem, &self->Handle);
        self->IsOpen = false;
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
    /* The contract does not distinguish a short read from an error, so a
     * negative result and a small one fail the same way. */
    return (read >= 0) && ((size_t) read == count);
}

static bool LittleFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    lfs_ssize_t written = lfs_file_write(self->Filesystem, &self->Handle, buf, (lfs_size_t) count);
    bool wroteAllData = (written >= 0) && ((size_t) written == count);
    /* The File contract makes a true return mean the bytes are on the media:
     * the BlockStore treats it as durable across power loss, so the sync is
     * part of the write rather than something a caller arranges. */
    return wroteAllData && (lfs_file_sync(self->Filesystem, &self->Handle) == LFS_ERR_OK);
}

static void LittleFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    /* The contract says seek errors are silent. */
    (void) lfs_file_seek(self->Filesystem, &self->Handle, (lfs_soff_t) offset, LFS_SEEK_SET);
}

static size_t LittleFsFile_Size(struct SolidSyslogFile* base)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    lfs_soff_t size = lfs_file_size(self->Filesystem, &self->Handle);
    /* lfs reports an error as a negative value, which the contract renders
     * as zero. */
    return (size > 0) ? (size_t) size : 0U;
}

static void LittleFsFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogLittleFsFile* self = LittleFsFile_SelfFromBase(base);
    /* Seek as well as truncate: the contract leaves the file open at length
     * zero, and the shared position would otherwise sit past the new end. */
    (void) lfs_file_truncate(self->Filesystem, &self->Handle, 0);
    (void) lfs_file_seek(self->Filesystem, &self->Handle, 0, LFS_SEEK_SET);
}

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
