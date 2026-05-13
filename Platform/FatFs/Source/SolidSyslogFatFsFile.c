#include "SolidSyslogFatFsFile.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogMacros.h"
#include "ff.h"

#define READ_WRITE_OR_CREATE (FA_READ | FA_WRITE | FA_OPEN_ALWAYS)

static bool                                FatFsFile_Open(struct SolidSyslogFile* self, const char* path);
static void                                FatFsFile_Close(struct SolidSyslogFile* self);
static bool                                FatFsFile_IsOpen(struct SolidSyslogFile* self);
static bool                                FatFsFile_Read(struct SolidSyslogFile* self, void* buf, size_t count);
static bool                                FatFsFile_Write(struct SolidSyslogFile* self, const void* buf, size_t count);
static void                                FatFsFile_SeekTo(struct SolidSyslogFile* self, size_t offset);
static size_t                              FatFsFile_Size(struct SolidSyslogFile* self);
static void                                FatFsFile_Truncate(struct SolidSyslogFile* self);
static bool                                FatFsFile_Exists(struct SolidSyslogFile* self, const char* path);
static bool                                FatFsFile_Delete(struct SolidSyslogFile* self, const char* path);
static inline struct SolidSyslogFatFsFile* FatFsFile_Self(struct SolidSyslogFile* self);
static inline FIL*                         FatFsFile_Handle(struct SolidSyslogFile* self);

struct SolidSyslogFatFsFile
{
    struct SolidSyslogFile base;
    FIL                    fp;
    bool                   isOpen;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogFatFsFile) <= sizeof(SolidSyslogFatFsFileStorage),
                          "SOLIDSYSLOG_FATFS_FILE_SIZE is too small for struct SolidSyslogFatFsFile");

static const struct SolidSyslogFatFsFile DEFAULT_INSTANCE = {
    .base   = {FatFsFile_Open, FatFsFile_Close, FatFsFile_IsOpen, FatFsFile_Read, FatFsFile_Write, FatFsFile_SeekTo, FatFsFile_Size, FatFsFile_Truncate,
               FatFsFile_Exists, FatFsFile_Delete},
    .isOpen = false,
};

struct SolidSyslogFile* SolidSyslogFatFsFile_Create(SolidSyslogFatFsFileStorage* storage)
{
    struct SolidSyslogFatFsFile* fatfs = (struct SolidSyslogFatFsFile*) storage;
    *fatfs                             = DEFAULT_INSTANCE;
    return &fatfs->base;
}

void SolidSyslogFatFsFile_Destroy(struct SolidSyslogFile* file)
{
    FatFsFile_Close(file);
}

static bool FatFsFile_Open(struct SolidSyslogFile* self, const char* path)
{
    struct SolidSyslogFatFsFile* fatfs  = FatFsFile_Self(self);
    FRESULT                      result = f_open(&fatfs->fp, path, READ_WRITE_OR_CREATE);
    fatfs->isOpen                       = (result == FR_OK);
    return fatfs->isOpen;
}

static inline struct SolidSyslogFatFsFile* FatFsFile_Self(struct SolidSyslogFile* self)
{
    return (struct SolidSyslogFatFsFile*) self;
}

static void FatFsFile_Close(struct SolidSyslogFile* self)
{
    struct SolidSyslogFatFsFile* fatfs = FatFsFile_Self(self);
    if (fatfs->isOpen)
    {
        f_close(&fatfs->fp);
        fatfs->isOpen = false;
    }
}

static bool FatFsFile_IsOpen(struct SolidSyslogFile* self)
{
    return FatFsFile_Self(self)->isOpen;
}

static bool FatFsFile_Read(struct SolidSyslogFile* self, void* buf, size_t count)
{
    UINT    bytesRead = 0;
    FRESULT result    = f_read(FatFsFile_Handle(self), buf, (UINT) count, &bytesRead);
    return (result == FR_OK) && (bytesRead == count);
}

static inline FIL* FatFsFile_Handle(struct SolidSyslogFile* self)
{
    return &FatFsFile_Self(self)->fp;
}

static bool FatFsFile_Write(struct SolidSyslogFile* self, const void* buf, size_t count)
{
    UINT    bytesWritten = 0;
    FRESULT result       = f_write(FatFsFile_Handle(self), buf, (UINT) count, &bytesWritten);
    bool    wroteAllData = (result == FR_OK) && (bytesWritten == count);
    return wroteAllData && (f_sync(FatFsFile_Handle(self)) == FR_OK);
}

static void FatFsFile_SeekTo(struct SolidSyslogFile* self, size_t offset)
{
    f_lseek(FatFsFile_Handle(self), (FSIZE_t) offset);
}

static size_t FatFsFile_Size(struct SolidSyslogFile* self)
{
    return (size_t) f_size(FatFsFile_Handle(self));
}

static void FatFsFile_Truncate(struct SolidSyslogFile* self)
{
    f_lseek(FatFsFile_Handle(self), 0);
    f_truncate(FatFsFile_Handle(self));
}

static bool FatFsFile_Exists(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return f_stat(path, NULL) == FR_OK;
}

static bool FatFsFile_Delete(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return f_unlink(path) == FR_OK;
}
