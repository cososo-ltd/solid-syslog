/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPlusFatFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPlusFatFileErrors.h"
#include "SolidSyslogPlusFatFilePrivate.h"
#include "ff_stdio.h"

const struct SolidSyslogErrorSource SolidSyslogPlusFatFileErrorSource = {"PlusFatFile", SolidSyslogFileErrors_Name};

static bool PlusFatFile_Open(struct SolidSyslogFile* base, const char* path);
static void PlusFatFile_Close(struct SolidSyslogFile* base);
static bool PlusFatFile_IsOpen(struct SolidSyslogFile* base);
static bool PlusFatFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool PlusFatFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void PlusFatFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static bool PlusFatFile_Size(struct SolidSyslogFile* base, size_t* size);
static void PlusFatFile_Truncate(struct SolidSyslogFile* base);
static bool PlusFatFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool PlusFatFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogPlusFatFile* PlusFatFile_SelfFromBase(struct SolidSyslogFile* base);
static inline bool PlusFatFile_IsFileOpen(const struct SolidSyslogPlusFatFile* self);
static inline int PlusFatFile_ErrnoUnlessOpen(const struct SolidSyslogPlusFatFile* self);
static inline bool PlusFatFile_IsAbsent(int nativeErrno);
static inline void PlusFatFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeCode);
static inline bool PlusFatFile_Flush(struct SolidSyslogPlusFatFile* self);

void SolidSyslogPlusFatFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    self->Base.Open = PlusFatFile_Open;
    self->Base.Close = PlusFatFile_Close;
    self->Base.IsOpen = PlusFatFile_IsOpen;
    self->Base.Read = PlusFatFile_Read;
    self->Base.Write = PlusFatFile_Write;
    self->Base.SeekTo = PlusFatFile_SeekTo;
    self->Base.Size = PlusFatFile_Size;
    self->Base.Truncate = PlusFatFile_Truncate;
    self->Base.Exists = PlusFatFile_Exists;
    self->Base.Delete = PlusFatFile_Delete;
    self->Fp = NULL;
}

static inline struct SolidSyslogPlusFatFile* PlusFatFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogPlusFatFile*) base;
}

void SolidSyslogPlusFatFile_Cleanup(struct SolidSyslogFile* base)
{
    PlusFatFile_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static void PlusFatFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    if (PlusFatFile_IsFileOpen(self))
    {
        int status = ff_fclose(self->Fp);
        /* The task errno is read straight after the call that set it. */
        int closeErrno = (status != 0) ? stdioGET_ERRNO() : 0;
        /* The file is released whether or not the close could flush. */
        self->Fp = NULL;
        if (status != 0)
        {
            PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, closeErrno);
        }
    }
}

/* Open-state is carried by the FF_FILE* sentinel: non-NULL once a file is open,
 * NULL when closed. The single source of truth for every open-state check. */
static inline bool PlusFatFile_IsFileOpen(const struct SolidSyslogPlusFatFile* self)
{
    return self->Fp != NULL;
}

/* The fault, with the code behind it when the call left one. The stdio calls
 * leave a FreeRTOS errno in the task; FF_FlushCache answers an FF_Error_t. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- every caller passes a named code and the code it read
static inline void PlusFatFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeCode)
{
    if (nativeCode != 0)
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
            &SolidSyslogPlusFatFileErrorSource,
            SOLIDSYSLOG_CAT_FILE_IO_FAILED,
            (int32_t) code,
            (int32_t) nativeCode
        );
    }
    else
    {
        PlusFatFile_Report(SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY, SOLIDSYSLOG_CAT_FILE_IO_FAILED, code);
    }
}

static bool PlusFatFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* "r+" opens an existing file without truncating. Fall back to the
     * file-creating "w+" only when the "r+" failure was specifically "file does
     * not exist" - "w+" truncates, so an "r+" failure from any other cause
     * (media/I/O error) on an existing file must never reach it, or it would
     * empty a record file the BlockStore believes is durably stored. The exact
     * reason lives in the task errno ff_fopen just set; ENOENT is the only code
     * for which creating is safe. Plus-FAT has no single non-truncating
     * open-or-create mode. */
    self->Fp = ff_fopen(path, "r+");
    int openErrno = PlusFatFile_ErrnoUnlessOpen(self);
    if (PlusFatFile_IsAbsent(openErrno))
    {
        self->Fp = ff_fopen(path, "w+");
        openErrno = PlusFatFile_ErrnoUnlessOpen(self);
    }
    if (!PlusFatFile_IsFileOpen(self))
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, openErrno);
    }
    return PlusFatFile_IsFileOpen(self);
}

/* The task errno the ff_fopen that just ran left behind, or none when it
 * opened the file. */
static inline int PlusFatFile_ErrnoUnlessOpen(const struct SolidSyslogPlusFatFile* self)
{
    return PlusFatFile_IsFileOpen(self) ? 0 : stdioGET_ERRNO();
}

/* True only when a call failed because the path does not exist. For ff_fopen it
 * is the single code for which falling back to the truncating "w+" cannot
 * destroy data; for ff_remove it means the path is already gone. */
static inline bool PlusFatFile_IsAbsent(int nativeErrno)
{
    return nativeErrno == pdFREERTOS_ERRNO_ENOENT;
}

static bool PlusFatFile_IsOpen(struct SolidSyslogFile* base)
{
    return PlusFatFile_IsFileOpen(PlusFatFile_SelfFromBase(base));
}

static bool PlusFatFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    /* xSize == 1 so the ff_fread return value is the byte count delivered. */
    size_t delivered = ff_fread(buf, 1, count, self->Fp);
    /* ff_fread records why it stopped short in the task errno: a failure
     * leaves its code, the end of the file leaves none and the store judges
     * that for itself. */
    int readErrno = (delivered == count) ? 0 : stdioGET_ERRNO();
    if (readErrno != 0)
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, readErrno);
    }
    return delivered == count;
}

static bool PlusFatFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    size_t written = ff_fwrite(buf, 1, count, self->Fp);
    /* A full volume is a short write with no errno; a failure leaves its code. */
    int writeErrno = (written == count) ? 0 : stdioGET_ERRNO();
    bool wroteAll = (written == count);
    if (wroteAll)
    {
        /* Flush the IO-manager cache after every complete write so the record's
         * data sectors reach the media. Plus-FAT has no per-file flush -
         * ff_stdio.h declares ff_fflush but the library never defines it;
         * FF_FlushCache against the file's IO manager is the real durability
         * primitive. It does not commit the directory entry: FF_Close writes the
         * recorded size, so a power loss before then leaves data the size
         * excludes. */
        wroteAll = PlusFatFile_Flush(self);
    }
    else
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, writeErrno);
    }
    return wroteAll;
}

/* Flush the IO-manager cache to the media; returns whether it succeeded. The
 * FF_Error_t it answers is the code behind a failure. */
static inline bool PlusFatFile_Flush(struct SolidSyslogPlusFatFile* self)
{
    FF_Error_t result = FF_FlushCache(self->Fp->pxIOManager);
    if (result != FF_ERR_NONE)
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SYNC_FAILED, (int) result);
    }
    return result == FF_ERR_NONE;
}

static void PlusFatFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    int status = ff_fseek(self->Fp, (long) offset, SEEK_SET);
    int seekErrno = (status != 0) ? stdioGET_ERRNO() : 0;
    if (status != 0)
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, seekErrno);
    }
}

static bool PlusFatFile_Size(struct SolidSyslogFile* base, size_t* size)
{
    /* ff_filelength answers 0 for a failure as well as for an empty file; the
     * task errno it sets on every call is what tells them apart. */
    size_t length = ff_filelength(PlusFatFile_SelfFromBase(base)->Fp);
    int sizeErrno = stdioGET_ERRNO();
    bool known = sizeErrno == pdFREERTOS_ERRNO_NONE;
    if (!known)
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, sizeErrno);
    }
    *size = known ? length : 0U;
    return known;
}

/* Empty the file: rewind to the start, then set EOF there so the length
 * becomes zero. ff_seteof truncates at the current position and Plus-FAT has
 * no truncate-to-zero call of its own, so the seek is part of the truncate:
 * one that fails leaves the file where it was, and is reported as the
 * truncate failing. */
static void PlusFatFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogPlusFatFile* self = PlusFatFile_SelfFromBase(base);
    int status = ff_fseek(self->Fp, 0, SEEK_SET);
    if (status == 0)
    {
        status = ff_seteof(self->Fp);
    }
    /* Nothing runs between whichever call failed and this read of its errno. */
    int truncateErrno = (status != 0) ? stdioGET_ERRNO() : 0;
    if (status != 0)
    {
        PlusFatFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, truncateErrno);
    }
}

/* A path that is not there is the answer, not a fault, so nothing is reported. */
static bool PlusFatFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    FF_Stat_t status;
    return ff_stat(path, &status) == 0;
}

static bool PlusFatFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    /* The contract counts an absent path as removed. */
    return (ff_remove(path) == 0) || PlusFatFile_IsAbsent(stdioGET_ERRNO());
}
