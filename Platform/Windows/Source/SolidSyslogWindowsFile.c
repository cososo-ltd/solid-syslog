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
    INVALID_FD             = -1,
    ACCESS_EXISTENCE_CHECK = 0 /* equivalent to POSIX F_OK; <io.h> does not define an alias */
};

static bool   Open(struct SolidSyslogFile* self, const char* path);
static void   Close(struct SolidSyslogFile* self);
static bool   IsOpen(struct SolidSyslogFile* self);
static bool   Read(struct SolidSyslogFile* self, void* buf, size_t count);
static bool   Write(struct SolidSyslogFile* self, const void* buf, size_t count);
static void   SeekTo(struct SolidSyslogFile* self, size_t offset);
static size_t Size(struct SolidSyslogFile* self);
static void   Truncate(struct SolidSyslogFile* self);
static bool   Exists(struct SolidSyslogFile* self, const char* path);
static bool   Delete(struct SolidSyslogFile* self, const char* path);

struct SolidSyslogWindowsFile
{
    struct SolidSyslogFile base;
    int                    fd;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogWindowsFile) <= sizeof(SolidSyslogWindowsFileStorage),
                          "SOLIDSYSLOG_WINDOWS_FILE_SIZE is too small for struct SolidSyslogWindowsFile");

static const struct SolidSyslogWindowsFile DEFAULT_INSTANCE = {
    {Open, Close, IsOpen, Read, Write, SeekTo, Size, Truncate, Exists, Delete},
    INVALID_FD,
};

static const struct SolidSyslogWindowsFile DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    INVALID_FD,
};

struct SolidSyslogFile* SolidSyslogWindowsFile_Create(SolidSyslogWindowsFileStorage* storage)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) storage;
    *windows                               = DEFAULT_INSTANCE;
    return &windows->base;
}

void SolidSyslogWindowsFile_Destroy(struct SolidSyslogFile* file)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) file;

    if (windows->fd != INVALID_FD)
    {
        _close(windows->fd);
    }

    *windows = DESTROYED_INSTANCE;
}

static bool Open(struct SolidSyslogFile* self, const char* path)
{
    /* _sopen_s is the non-deprecated MSVC equivalent of POSIX open(): the
     * plain _open triggers C4996 (Microsoft's safe-CRT preference) and
     * _CRT_SECURE_NO_WARNINGS is forbidden by the project's banned-API
     * policy. _SH_DENYNO matches POSIX open()'s default of no share mode. */
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    errno_t                        err     = _sopen_s(&windows->fd, path, DEFAULT_OPEN_FLAGS, _SH_DENYNO, DEFAULT_FILE_PERMISSIONS);
    if (err != 0)
    {
        windows->fd = INVALID_FD;
    }
    return windows->fd != INVALID_FD;
}

static void Close(struct SolidSyslogFile* self)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;

    if (windows->fd != INVALID_FD)
    {
        _close(windows->fd);
        windows->fd = INVALID_FD;
    }
}

static bool IsOpen(struct SolidSyslogFile* self)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    return windows->fd != INVALID_FD;
}

static bool Read(struct SolidSyslogFile* self, void* buf, size_t count)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    return _read(windows->fd, buf, (unsigned int) count) == (int) count;
}

static bool Write(struct SolidSyslogFile* self, const void* buf, size_t count)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    return _write(windows->fd, buf, (unsigned int) count) == (int) count;
}

static void SeekTo(struct SolidSyslogFile* self, size_t offset)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    _lseeki64(windows->fd, (__int64) offset, SEEK_SET);
}

static size_t Size(struct SolidSyslogFile* self)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    __int64                        size    = _lseeki64(windows->fd, 0, SEEK_END);
    return (size >= 0) ? (size_t) size : 0;
}

static void Truncate(struct SolidSyslogFile* self)
{
    struct SolidSyslogWindowsFile* windows = (struct SolidSyslogWindowsFile*) self;
    _chsize_s(windows->fd, 0);
}

static bool Exists(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return _access(path, ACCESS_EXISTENCE_CHECK) == 0;
}

static bool Delete(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return _unlink(path) == 0;
}
