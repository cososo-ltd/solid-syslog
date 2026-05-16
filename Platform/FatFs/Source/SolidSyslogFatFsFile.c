#include "SolidSyslogFatFsFile.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogMacros.h"
#include "ff.h"

#define READ_WRITE_OR_CREATE (FA_READ | FA_WRITE | FA_OPEN_ALWAYS)

static bool FatFsFile_Open(struct SolidSyslogFile* base, const char* path);
static void FatFsFile_Close(struct SolidSyslogFile* base);
static bool FatFsFile_IsOpen(struct SolidSyslogFile* base);
static bool FatFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool FatFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void FatFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static size_t FatFsFile_Size(struct SolidSyslogFile* base);
static void FatFsFile_Truncate(struct SolidSyslogFile* base);
static bool FatFsFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool FatFsFile_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromStorage(SolidSyslogFatFsFileStorage* storage);
static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromBase(struct SolidSyslogFile* base);
static inline FIL* FatFsFile_Handle(struct SolidSyslogFile* base);

struct SolidSyslogFatFsFile
{
    struct SolidSyslogFile Base;
    FIL Fp;
    bool IsOpen;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogFatFsFile) <= sizeof(SolidSyslogFatFsFileStorage),
    "SOLIDSYSLOG_FATFS_FILE_SIZE is too small for struct SolidSyslogFatFsFile"
);

static const struct SolidSyslogFatFsFile DEFAULT_INSTANCE = {
    .Base =
        {FatFsFile_Open,
         FatFsFile_Close,
         FatFsFile_IsOpen,
         FatFsFile_Read,
         FatFsFile_Write,
         FatFsFile_SeekTo,
         FatFsFile_Size,
         FatFsFile_Truncate,
         FatFsFile_Exists,
         FatFsFile_Delete},
    .IsOpen = false,
};

struct SolidSyslogFile* SolidSyslogFatFsFile_Create(SolidSyslogFatFsFileStorage* storage)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;
    return &self->Base;
}

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromStorage(SolidSyslogFatFsFileStorage* storage)
{
    return (struct SolidSyslogFatFsFile*) storage;
}

void SolidSyslogFatFsFile_Destroy(struct SolidSyslogFile* base)
{
    FatFsFile_Close(base);
}

static bool FatFsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    FRESULT result = f_open(&self->Fp, path, READ_WRITE_OR_CREATE);
    self->IsOpen = (result == FR_OK);
    return self->IsOpen;
}

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogFatFsFile*) base;
}

static void FatFsFile_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    if (self->IsOpen)
    {
        f_close(&self->Fp);
        self->IsOpen = false;
    }
}

static bool FatFsFile_IsOpen(struct SolidSyslogFile* base)
{
    return FatFsFile_SelfFromBase(base)->IsOpen;
}

static bool FatFsFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    UINT bytesRead = 0;
    FRESULT result = f_read(FatFsFile_Handle(base), buf, (UINT) count, &bytesRead);
    return (result == FR_OK) && (bytesRead == count);
}

static inline FIL* FatFsFile_Handle(struct SolidSyslogFile* base)
{
    return &FatFsFile_SelfFromBase(base)->Fp;
}

static bool FatFsFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    UINT bytesWritten = 0;
    FRESULT result = f_write(FatFsFile_Handle(base), buf, (UINT) count, &bytesWritten);
    bool wroteAllData = (result == FR_OK) && (bytesWritten == count);
    return wroteAllData && (f_sync(FatFsFile_Handle(base)) == FR_OK);
}

static void FatFsFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    f_lseek(FatFsFile_Handle(base), (FSIZE_t) offset);
}

static size_t FatFsFile_Size(struct SolidSyslogFile* base)
{
    return (size_t) f_size(FatFsFile_Handle(base));
}

static void FatFsFile_Truncate(struct SolidSyslogFile* base)
{
    f_lseek(FatFsFile_Handle(base), 0);
    f_truncate(FatFsFile_Handle(base));
}

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
