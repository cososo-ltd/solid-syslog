#include "SolidSyslogWindowsFile.h"

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogWindowsFilePrivate.h"

/* _O_BINARY disables the MSVC CRT's CR/LF translation on read/write so
 * arbitrary binary content (e.g. SolidSyslogBlockStore frames) round-trips
 * unchanged. Without it the CRT substitutes 0x0D 0x0A for 0x0A on write
 * and the inverse on read, corrupting any byte that happens to fall on
 * those values. */
#define DEFAULT_OPEN_FLAGS (_O_RDWR | _O_CREAT | _O_BINARY)
#define DEFAULT_FILE_PERMISSIONS (_S_IREAD | _S_IWRITE)

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
static size_t WindowsFile_Size(struct SolidSyslogFile* base);
static void WindowsFile_Truncate(struct SolidSyslogFile* base);
static bool WindowsFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool WindowsFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromBase(struct SolidSyslogFile* base);

void WindowsFile_Initialise(struct SolidSyslogFile* base)
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

void WindowsFile_Cleanup(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    if (self->Fd != INVALID_FD)
    {
        _close(self->Fd);
        self->Fd = INVALID_FD;
    }
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogWindowsFile*) base;
}

static bool WindowsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    /* _sopen_s is the non-deprecated MSVC equivalent of POSIX open(): the
     * plain _open triggers C4996 (Microsoft's safe-CRT preference) and
     * _CRT_SECURE_NO_WARNINGS is forbidden by the project's banned-API
     * policy. _SH_DENYNO matches POSIX open()'s default of no share mode. */
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    errno_t err = _sopen_s(&self->Fd, path, DEFAULT_OPEN_FLAGS, _SH_DENYNO, DEFAULT_FILE_PERMISSIONS);
    if (err != 0)
    {
        self->Fd = INVALID_FD;
    }
    return self->Fd != INVALID_FD;
}

static void WindowsFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);

    if (self->Fd != INVALID_FD)
    {
        _close(self->Fd);
        self->Fd = INVALID_FD;
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
    return _read(self->Fd, buf, (unsigned int) count) == (int) count;
}

static bool WindowsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    return _write(self->Fd, buf, (unsigned int) count) == (int) count;
}

static void WindowsFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    _lseeki64(self->Fd, (__int64) offset, SEEK_SET);
}

static size_t WindowsFile_Size(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    __int64 size = _lseeki64(self->Fd, 0, SEEK_END);
    return (size >= 0) ? (size_t) size : 0U;
}

static void WindowsFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);
    _chsize_s(self->Fd, 0);
}

static bool WindowsFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return _access(path, ACCESS_EXISTENCE_CHECK) == 0;
}

static bool WindowsFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    return _unlink(path) == 0;
}
