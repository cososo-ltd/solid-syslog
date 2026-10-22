/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFatFsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFatFsFileErrors.h"
#include "SolidSyslogFatFsFilePrivate.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "ff.h"

const struct SolidSyslogErrorSource SolidSyslogFatFsFileErrorSource = {"FatFsFile", SolidSyslogFileErrors_Name};

#define FATFSFILE_READ_WRITE_OR_CREATE (FA_READ | FA_WRITE | FA_OPEN_ALWAYS)

static bool FatFsFile_Open(struct SolidSyslogFile* base, const char* path);
static void FatFsFile_Close(struct SolidSyslogFile* base);
static bool FatFsFile_IsOpen(struct SolidSyslogFile* base);
static bool FatFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool FatFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void FatFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static bool FatFsFile_Size(struct SolidSyslogFile* base, size_t* size);
static void FatFsFile_Truncate(struct SolidSyslogFile* base);
static bool FatFsFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool FatFsFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromBase(struct SolidSyslogFile* base);
static inline FIL* FatFsFile_Handle(struct SolidSyslogFile* base);
static inline void FatFsFile_ReportFailure(enum SolidSyslogFileErrors code, FRESULT result);
static inline bool FatFsFile_Sync(struct SolidSyslogFile* base);
static inline bool FatFsFile_IsCloseStillOwed(FRESULT result);
static bool FatFsFile_SettleOwedClose(struct SolidSyslogFatFsFile* self);

void SolidSyslogFatFsFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    self->Base.Open = FatFsFile_Open;
    self->Base.Close = FatFsFile_Close;
    self->Base.IsOpen = FatFsFile_IsOpen;
    self->Base.Read = FatFsFile_Read;
    self->Base.Write = FatFsFile_Write;
    self->Base.SeekTo = FatFsFile_SeekTo;
    self->Base.Size = FatFsFile_Size;
    self->Base.Truncate = FatFsFile_Truncate;
    self->Base.Exists = FatFsFile_Exists;
    self->Base.Delete = FatFsFile_Delete;
    self->IsOpen = false;
    self->CloseOwed = false;
}

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogFatFsFile*) base;
}

/* A close still owed is tried once more; the slot is given back whatever it
 * answers. */
void SolidSyslogFatFsFile_Cleanup(struct SolidSyslogFile* base)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    if (self->IsOpen)
    {
        FatFsFile_Close(base);
    }
    else if (self->CloseOwed)
    {
        (void) FatFsFile_SettleOwedClose(self);
    }
    else
    {
        /* closed, and nothing owed */
    }
    self->CloseOwed = false;
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static void FatFsFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    if (self->IsOpen)
    {
        FRESULT result = f_close(&self->Fp);
        /* Closed, as the contract says, whatever f_close answers. A failed
         * sync leaves FatFs holding the file object, so the close is owed. */
        self->IsOpen = false;
        self->CloseOwed = FatFsFile_IsCloseStillOwed(result);
        if (result != FR_OK)
        {
            FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, result);
        }
    }
}

/* FR_INVALID_OBJECT is FatFs saying it no longer knows the file object - a
 * remount discards every one, and its locks with them - so there is nothing
 * left for a later f_close to release. */
static inline bool FatFsFile_IsCloseStillOwed(FRESULT result)
{
    return (result != FR_OK) && (result != FR_INVALID_OBJECT);
}

/* The fault, with the FRESULT behind it when the call answered one. A short
 * transfer that FatFs answers FR_OK leaves no code. */
static inline void FatFsFile_ReportFailure(enum SolidSyslogFileErrors code, FRESULT result)
{
    if (result != FR_OK)
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
            &SolidSyslogFatFsFileErrorSource,
            SOLIDSYSLOG_CAT_FILE_IO_FAILED,
            (int32_t) code,
            (int32_t) result
        );
    }
    else
    {
        FatFsFile_Report(SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY, SOLIDSYSLOG_CAT_FILE_IO_FAILED, code);
    }
}

static bool FatFsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    bool opened = false;
    /* An owed close is settled before the file object is reused: f_open would
     * discard what FatFs still holds in it, and its FF_FS_LOCK entry with it.
     * One that cannot be settled refuses the open, and the file stays closed
     * with the close still owed for the next one. */
    if (FatFsFile_SettleOwedClose(self))
    {
        FRESULT result = f_open(&self->Fp, path, FATFSFILE_READ_WRITE_OR_CREATE);
        self->IsOpen = (result == FR_OK);
        if (!self->IsOpen)
        {
            FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, result);
        }
        opened = self->IsOpen;
    }
    return opened;
}

/* True when nothing is owed, or the owed close has now gone through. A retry
 * that fails again is reported again. */
static bool FatFsFile_SettleOwedClose(struct SolidSyslogFatFsFile* self)
{
    if (self->CloseOwed)
    {
        FRESULT result = f_close(&self->Fp);
        self->CloseOwed = FatFsFile_IsCloseStillOwed(result);
        if (self->CloseOwed)
        {
            FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, result);
        }
    }
    return !self->CloseOwed;
}

static bool FatFsFile_IsOpen(struct SolidSyslogFile* base)
{
    return FatFsFile_SelfFromBase(base)->IsOpen;
}

static bool FatFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    UINT bytesRead = 0U;
    FRESULT result = f_read(FatFsFile_Handle(base), buf, (UINT) count, &bytesRead);
    /* A short read is FR_OK with fewer bytes; the store judges it. */
    if (result != FR_OK)
    {
        FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, result);
    }
    return (result == FR_OK) && (bytesRead == count);
}

static inline FIL* FatFsFile_Handle(struct SolidSyslogFile* base)
{
    return &FatFsFile_SelfFromBase(base)->Fp;
}

static bool FatFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    UINT bytesWritten = 0U;
    FRESULT result = f_write(FatFsFile_Handle(base), buf, (UINT) count, &bytesWritten);
    /* A full volume is FR_OK with fewer bytes: a failed write with no code. */
    bool wroteAllData = (result == FR_OK) && (bytesWritten == count);
    if (wroteAllData)
    {
        wroteAllData = FatFsFile_Sync(base);
    }
    else
    {
        FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, result);
    }
    return wroteAllData;
}

static inline bool FatFsFile_Sync(struct SolidSyslogFile* base)
{
    FRESULT result = f_sync(FatFsFile_Handle(base));
    if (result != FR_OK)
    {
        FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SYNC_FAILED, result);
    }
    return result == FR_OK;
}

static void FatFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    FRESULT result = f_lseek(FatFsFile_Handle(base), (FSIZE_t) offset);
    if (result != FR_OK)
    {
        FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, result);
    }
}

static bool FatFsFile_Size(struct SolidSyslogFile* base, size_t* size)
{
    /* f_size reads the file object's own record of its length and cannot fail. */
    *size = (size_t) f_size(FatFsFile_Handle(base));
    return true;
}

/* f_truncate cuts the file at its current position, so the seek to the start
 * is part of the truncate: a seek that fails leaves the file where it was,
 * and is reported as the truncate failing. */
static void FatFsFile_Truncate(struct SolidSyslogFile* base)
{
    FRESULT result = f_lseek(FatFsFile_Handle(base), 0);
    if (result == FR_OK)
    {
        result = f_truncate(FatFsFile_Handle(base));
    }
    if (result != FR_OK)
    {
        FatFsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, result);
    }
}

/* A path that is not there is the answer, not a fault, so nothing is reported. */
static bool FatFsFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return f_stat(path, NULL) == FR_OK;
}

static bool FatFsFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return f_unlink(path) == FR_OK;
}
