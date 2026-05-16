#include "SolidSyslogWindowsFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogMacros.h"

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <sys/stat.h>

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

static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromStorage(SolidSyslogWindowsFileStorage* storage);
static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromBase(struct SolidSyslogFile* base);

struct SolidSyslogWindowsFile
{
    struct SolidSyslogFile Base;
    int Fd;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogWindowsFile) <= sizeof(SolidSyslogWindowsFileStorage),
    "SOLIDSYSLOG_WINDOWS_FILE_SIZE is too small for struct SolidSyslogWindowsFile"
);

static const struct SolidSyslogWindowsFile DEFAULT_INSTANCE = {
    {WindowsFile_Open,
     WindowsFile_Close,
     WindowsFile_IsOpen,
     WindowsFile_Read,
     WindowsFile_Write,
     WindowsFile_SeekTo,
     WindowsFile_Size,
     WindowsFile_Truncate,
     WindowsFile_Exists,
     WindowsFile_Delete},
    INVALID_FD,
};

static const struct SolidSyslogWindowsFile DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    INVALID_FD,
};

struct SolidSyslogFile* SolidSyslogWindowsFile_Create(SolidSyslogWindowsFileStorage* storage)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;
    return &self->Base;
}

static inline struct SolidSyslogWindowsFile* WindowsFile_SelfFromStorage(SolidSyslogWindowsFileStorage* storage)
{
    return (struct SolidSyslogWindowsFile*) storage;
}

void SolidSyslogWindowsFile_Destroy(struct SolidSyslogFile* base)
{
    struct SolidSyslogWindowsFile* self = WindowsFile_SelfFromBase(base);

    if (self->Fd != INVALID_FD)
    {
        _close(self->Fd);
    }

    *self = DESTROYED_INSTANCE;
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
