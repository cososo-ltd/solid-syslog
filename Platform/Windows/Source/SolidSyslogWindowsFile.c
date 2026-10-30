/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsFile.h"

#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogWindowsFileErrors.h"
#include "SolidSyslogWindowsFilePrivate.h"

const struct SolidSyslogErrorSource SolidSyslogWindowsFileErrorSource = {"WindowsFile", SolidSyslogFileErrors_Name};

/* _O_BINARY disables the MSVC CRT's CR/LF translation on read/write so
 * arbitrary binary content (e.g. SolidSyslogBlockStore frames) round-trips
 * unchanged. Without it the CRT substitutes 0x0D 0x0A for 0x0A on write
 * and the inverse on read, corrupting any byte that happens to fall on
 * those values. */
#define WINDOWSFILE_DEFAULT_OPEN_FLAGS (_O_RDWR | _O_CREAT | _O_BINARY)
#define WINDOWSFILE_DEFAULT_PERMISSIONS (_S_IREAD | _S_IWRITE)

enum
{
    INVALID_FD = -1,
    ACCESS_EXISTENCE_CHECK = 0 /* equivalent to POSIX F_OK; <io.h> does not define an alias */
};

static bool WindowsFile_Open(struct SolidSyslogFile* base, const char* path);
static void WindowsFile_Close(struct SolidSyslogFile* base);
static bool WindowsFile_IsOpen(struct SolidSyslogFile* base);
static bool WindowsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool WindowsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void WindowsFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static bool WindowsFile_Size(struct SolidSyslogFile* base, size_t* size);
static void WindowsFile_Truncate(struct SolidSyslogFile* base);
static bool WindowsFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool WindowsFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromBase(struct SolidSyslogFile* base);
static inline void WindowsFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeErrno);
static inline bool WindowsFile_IsWholeTransfer(int transferred, size_t count);

void SolidSyslogWindowsFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    self->Base.Open = WindowsFile_Open;
    self->Base.Close = WindowsFile_Close;
    self->Base.IsOpen = WindowsFile_IsOpen;
    self->Base.Read = WindowsFile_Read;
    self->Base.Write = WindowsFile_Write;
    self->Base.SeekTo = WindowsFile_SeekTo;
    self->Base.Size = WindowsFile_Size;
    self->Base.Truncate = WindowsFile_Truncate;
    self->Base.Exists = WindowsFile_Exists;
    self->Base.Delete = WindowsFile_Delete;
    self->Fd = INVALID_FD;
}

void SolidSyslogWindowsFile_Cleanup(struct SolidSyslogFile* base)
{
    WindowsFile_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogWindowsFile*) base;
}

/* _sopen_s answers the errno itself rather than setting it. */
static bool WindowsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    errno_t err =
        _sopen_s(&self->Fd, path, WINDOWSFILE_DEFAULT_OPEN_FLAGS, _SH_DENYNO, WINDOWSFILE_DEFAULT_PERMISSIONS);
    if (err != 0)
    {
        self->Fd = INVALID_FD;
        WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, (int) err);
    }
    return self->Fd != INVALID_FD;
}

/* The fault, with the CRT errno behind it when the call left one. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- every caller passes a named code and the errno it read
static inline void WindowsFile_ReportFailure(enum SolidSyslogFileErrors code, int nativeErrno)
{
    if (nativeErrno != 0)
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
            &SolidSyslogWindowsFileErrorSource,
            SOLIDSYSLOG_CAT_FILE_IO_FAILED,
            (int32_t) code,
            (int32_t) nativeErrno
        );
    }
    else
    {
        WindowsFile_Report(SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY, SOLIDSYSLOG_CAT_FILE_IO_FAILED, code);
    }
}

static void WindowsFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    if (WindowsFile_IsOpen(base))
    {
        int status = _close(self->Fd);
        /* Read errno straight after the call that set it, with nothing between
         * (MISRA 22.10). */
        int closeErrno = (status != 0) ? errno : 0;
        /* The descriptor is released whether or not the close could flush. */
        self->Fd = INVALID_FD;
        if (status != 0)
        {
            WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, closeErrno);
        }
    }
}

static bool WindowsFile_IsOpen(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    return self->Fd != INVALID_FD;
}

static bool WindowsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    int transferred = _read(self->Fd, buf, (unsigned int) count);
    /* A short read sets no errno; the store judges it. */
    int readErrno = (transferred < 0) ? errno : 0;
    if (transferred < 0)
    {
        WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, readErrno);
    }
    return WindowsFile_IsWholeTransfer(transferred, count);
}

/* The CRT's _read and _write answer -1, or how many bytes moved. */
static inline bool WindowsFile_IsWholeTransfer(int transferred, size_t count)
{
    return (transferred >= 0) && ((size_t) transferred == count);
}

static bool WindowsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    int written = _write(self->Fd, buf, (unsigned int) count);
    /* A short write sets no errno. */
    int writeErrno = (written < 0) ? errno : 0;
    bool wrote = WindowsFile_IsWholeTransfer(written, count);
    if (!wrote)
    {
        WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, writeErrno);
    }
    return wrote;
}

static void WindowsFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    __int64 position = _lseeki64(self->Fd, (__int64) offset, SEEK_SET);
    int seekErrno = (position < 0) ? errno : 0;
    if (position < 0)
    {
        WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, seekErrno);
    }
}

static bool WindowsFile_Size(struct SolidSyslogFile* base, size_t* size)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    __int64 end = _lseeki64(self->Fd, 0, SEEK_END);
    bool known = end >= 0;
    int sizeErrno = known ? 0 : errno;
    if (!known)
    {
        WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, sizeErrno);
    }
    *size = known ? (size_t) end : 0U;
    return known;
}

/* _chsize_s answers the errno itself rather than setting it. */
static void WindowsFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    errno_t err = _chsize_s(self->Fd, 0);
    if (err != 0)
    {
        WindowsFile_ReportFailure(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, (int) err);
    }
}

/* A path that is not there is the answer, not a fault, so nothing is reported. */
static bool WindowsFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return _access(path, ACCESS_EXISTENCE_CHECK) == 0;
}

static bool WindowsFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    /* The contract counts an absent path as removed. */
    return (_unlink(path) == 0) || (errno == ENOENT);
}
