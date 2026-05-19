#include "SolidSyslogPosixFile.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPosixFilePrivate.h"

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

static inline struct SolidSyslogPosixFile* PosixFile_SelfFromBase(struct SolidSyslogFile* base);

void PosixFile_Initialise(struct SolidSyslogFile* base)
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

void PosixFile_Cleanup(struct SolidSyslogFile* base)
{
    struct SolidSyslogPosixFile* self = PosixFile_SelfFromBase(base);
    if (self->Fd != INVALID_FD)
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
    }
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
