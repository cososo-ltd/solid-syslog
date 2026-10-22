/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixFile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPosixFileErrors.h"
#include "SolidSyslogPosixFilePrivate.h"

const struct SolidSyslogErrorSource SolidSyslogPosixFileErrorSource = {"PosixFile", SolidSyslogFileErrors_Name};

#define POSIXFILE_OWNER_READ_WRITE (S_IRUSR | S_IWUSR)
#define POSIXFILE_DEFAULT_PERMISSIONS POSIXFILE_OWNER_READ_WRITE

enum
{
    INVALID_FD = -1
};

static bool PosixFile_Open(struct SolidSyslogFile* base, const char* path);
static void PosixFile_Close(struct SolidSyslogFile* base);
static bool PosixFile_IsOpen(struct SolidSyslogFile* base);
static bool PosixFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool PosixFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void PosixFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static bool PosixFile_Size(struct SolidSyslogFile* base, size_t* size);
static void PosixFile_Truncate(struct SolidSyslogFile* base);
static bool PosixFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool PosixFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogPosixFile* PosixFile_SelfFromBase(struct SolidSyslogFile* base);
static inline void PosixFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeErrno);
static inline bool PosixFile_IsWholeTransfer(ssize_t transferred, size_t count);

void SolidSyslogPosixFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    self->Base.Open = PosixFile_Open;
    self->Base.Close = PosixFile_Close;
    self->Base.IsOpen = PosixFile_IsOpen;
    self->Base.Read = PosixFile_Read;
    self->Base.Write = PosixFile_Write;
    self->Base.SeekTo = PosixFile_SeekTo;
    self->Base.Size = PosixFile_Size;
    self->Base.Truncate = PosixFile_Truncate;
    self->Base.Exists = PosixFile_Exists;
    self->Base.Delete = PosixFile_Delete;
    self->Fd = INVALID_FD;
}

void SolidSyslogPosixFile_Cleanup(struct SolidSyslogFile* base)
{
    PosixFile_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static inline struct SolidSyslogPosixFile* PosixFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogPosixFile*) base;
}

static bool PosixFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    self->Fd = open(path, O_RDWR | O_CREAT, POSIXFILE_DEFAULT_PERMISSIONS);
    /* Read errno straight after the call that set it, with nothing between
     * (MISRA 22.10). */
    int openErrno = (self->Fd == INVALID_FD) ? errno : 0;
    bool opened = self->Fd != INVALID_FD;
    if (!opened)
    {
        PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, openErrno);
    }
    return opened;
}

/* The fault, with the errno behind it when the call left one. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- every caller passes a named code and the errno it read
static inline void PosixFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeErrno)
{
    if (nativeErrno != 0)
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
            &SolidSyslogPosixFileErrorSource,
            SOLIDSYSLOG_CAT_FILE_IO_FAILED,
            (int32_t) code,
            (int32_t) nativeErrno
        );
    }
    else
    {
        PosixFile_Report(SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY, SOLIDSYSLOG_CAT_FILE_IO_FAILED, code);
    }
}

static void PosixFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    if (PosixFile_IsOpen(base))
    {
        int status = close(self->Fd);
        int closeErrno = (status != 0) ? errno : 0;
        /* The descriptor is released whether or not the close could flush. */
        self->Fd = INVALID_FD;
        if (status != 0)
        {
            PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, closeErrno);
        }
    }
}

static bool PosixFile_IsOpen(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    return self->Fd != INVALID_FD;
}

static bool PosixFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    ssize_t transferred = read(self->Fd, buf, count);
    /* A short read sets no errno; the store judges it. */
    int readErrno = (transferred < 0) ? errno : 0;
    if (transferred < 0)
    {
        PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, readErrno);
    }
    return PosixFile_IsWholeTransfer(transferred, count);
}

/* read and write answer -1, or how many bytes moved. */
static inline bool PosixFile_IsWholeTransfer(ssize_t transferred, size_t count)
{
    return (transferred >= 0) && ((size_t) transferred == count);
}

static bool PosixFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    ssize_t written = write(self->Fd, buf, count);
    /* A short write sets no errno. */
    int writeErrno = (written < 0) ? errno : 0;
    bool wrote = PosixFile_IsWholeTransfer(written, count);
    if (!wrote)
    {
        PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, writeErrno);
    }
    return wrote;
}

static void PosixFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    off_t position = lseek(self->Fd, (off_t) offset, SEEK_SET);
    int seekErrno = (position < 0) ? errno : 0;
    if (position < 0)
    {
        PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, seekErrno);
    }
}

static bool PosixFile_Size(struct SolidSyslogFile* base, size_t* size)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    off_t end = lseek(self->Fd, 0, SEEK_END);
    bool known = end >= 0;
    int sizeErrno = known ? 0 : errno;
    if (!known)
    {
        PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, sizeErrno);
    }
    *size = known ? (size_t) end : 0U;
    return known;
}

static void PosixFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    int status = ftruncate(self->Fd, 0);
    int truncateErrno = (status != 0) ? errno : 0;
    if (status != 0)
    {
        PosixFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, truncateErrno);
    }
}

/* A path that is not there is the answer, not a fault, so nothing is reported. */
static bool PosixFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return access(path, F_OK) == 0;
}

static bool PosixFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return unlink(path) == 0;
}
