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

static bool PosixFile_Open(struct SolidSyslogFile* base, const char* path);
static void PosixFile_Close(struct SolidSyslogFile* base);
static bool PosixFile_IsOpen(struct SolidSyslogFile* base);
static bool PosixFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool PosixFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void PosixFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static size_t PosixFile_Size(struct SolidSyslogFile* base);
static void PosixFile_Truncate(struct SolidSyslogFile* base);
static bool PosixFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool PosixFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogPosixFile* PosixFile_SelfFromStorage(SolidSyslogPosixFileStorage* storage);
static inline struct SolidSyslogPosixFile* PosixFile_SelfFromBase(struct SolidSyslogFile* base);

struct SolidSyslogPosixFile
{
    struct SolidSyslogFile Base;
    int Fd;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogPosixFile) <= sizeof(SolidSyslogPosixFileStorage),
    "SOLIDSYSLOG_POSIX_FILE_SIZE is too small for struct SolidSyslogPosixFile"
);

static const struct SolidSyslogPosixFile DEFAULT_INSTANCE = {
    {PosixFile_Open,
     PosixFile_Close,
     PosixFile_IsOpen,
     PosixFile_Read,
     PosixFile_Write,
     PosixFile_SeekTo,
     PosixFile_Size,
     PosixFile_Truncate,
     PosixFile_Exists,
     PosixFile_Delete},
    INVALID_FD,
};

static const struct SolidSyslogPosixFile DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    INVALID_FD,
};

struct SolidSyslogFile* SolidSyslogPosixFile_Create(SolidSyslogPosixFileStorage* storage)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;
    return &self->Base;
}

static inline struct SolidSyslogPosixFile* PosixFile_SelfFromStorage(SolidSyslogPosixFileStorage* storage)
{
    return (struct SolidSyslogPosixFile*) storage;
}

void SolidSyslogPosixFile_Destroy(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);

    if (self->Fd != INVALID_FD)
    {
        close(self->Fd);
    }

    *self = DESTROYED_INSTANCE;
}

static inline struct SolidSyslogPosixFile* PosixFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogPosixFile*) base;
}

static bool PosixFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    self->Fd = open(path, O_RDWR | O_CREAT, DEFAULT_FILE_PERMISSIONS);
    return self->Fd != INVALID_FD;
}

static void PosixFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);

    if (self->Fd != INVALID_FD)
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
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
    return read(self->Fd, buf, count) == (ssize_t) count;
}

static bool PosixFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    return write(self->Fd, buf, count) == (ssize_t) count;
}

static void PosixFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    lseek(self->Fd, (off_t) offset, SEEK_SET);
}

static size_t PosixFile_Size(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    off_t size = lseek(self->Fd, 0, SEEK_END);
    return (size >= 0) ? (size_t) size : 0U;
}

static void PosixFile_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    ftruncate(self->Fd, 0);
}

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
