#include "SolidSyslogPosixFile.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogMacros.h"

#define OWNER_READ_WRITE (S_IRUSR | S_IWUSR)
#define DEFAULT_FILE_PERMISSIONS OWNER_READ_WRITE

enum
{
    INVALID_FD = -1
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

struct SolidSyslogPosixFile
{
    struct SolidSyslogFile base;
    int                    fd;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogPosixFile) <= sizeof(SolidSyslogPosixFileStorage),
                          "SOLIDSYSLOG_POSIX_FILE_SIZE is too small for struct SolidSyslogPosixFile");

static const struct SolidSyslogPosixFile DEFAULT_INSTANCE = {
    {Open, Close, IsOpen, Read, Write, SeekTo, Size, Truncate, Exists, Delete},
    INVALID_FD,
};

static const struct SolidSyslogPosixFile DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    INVALID_FD,
};

struct SolidSyslogFile* SolidSyslogPosixFile_Create(SolidSyslogPosixFileStorage* storage)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) storage;
    *posix                             = DEFAULT_INSTANCE;
    return &posix->base;
}

void SolidSyslogPosixFile_Destroy(struct SolidSyslogFile* file)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) file;

    if (posix->fd != INVALID_FD)
    {
        close(posix->fd);
    }

    *posix = DESTROYED_INSTANCE;
}

static bool Open(struct SolidSyslogFile* self, const char* path)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    posix->fd                          = open(path, O_RDWR | O_CREAT, DEFAULT_FILE_PERMISSIONS);
    return posix->fd != INVALID_FD;
}

static void Close(struct SolidSyslogFile* self)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;

    if (posix->fd != INVALID_FD)
    {
        close(posix->fd);
        posix->fd = INVALID_FD;
    }
}

static bool IsOpen(struct SolidSyslogFile* self)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    return posix->fd != INVALID_FD;
}

static bool Read(struct SolidSyslogFile* self, void* buf, size_t count)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    return read(posix->fd, buf, count) == (ssize_t) count;
}

static bool Write(struct SolidSyslogFile* self, const void* buf, size_t count)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    return write(posix->fd, buf, count) == (ssize_t) count;
}

static void SeekTo(struct SolidSyslogFile* self, size_t offset)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    lseek(posix->fd, (off_t) offset, SEEK_SET);
}

static size_t Size(struct SolidSyslogFile* self)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    off_t                        size  = lseek(posix->fd, 0, SEEK_END);
    return (size >= 0) ? (size_t) size : 0;
}

static void Truncate(struct SolidSyslogFile* self)
{
    struct SolidSyslogPosixFile* posix = (struct SolidSyslogPosixFile*) self;
    ftruncate(posix->fd, 0);
}

static bool Exists(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return access(path, F_OK) == 0;
}

static bool Delete(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return unlink(path) == 0;
}
