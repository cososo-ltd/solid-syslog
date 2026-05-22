#include "SolidSyslogFatFsFile.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogFatFsFilePrivate.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
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

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromBase(struct SolidSyslogFile* base);
static inline FIL* FatFsFile_Handle(struct SolidSyslogFile* base);

void FatFsFile_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    self->Base.Open = FatFsFile_Open;
    self->Base.Close = FatFsFile_Close;
    self->Base.IsOpen = FatFsFile_IsOpen;
    self->Base.Read = FatFsFile_Read;
    self->Base.Write = FatFsFile_Write;
    self->Base.SeekTo = FatFsFile_SeekTo;
    self->Base.Size = FatFsFile_Size;
    self->Base.Truncate = FatFsFile_Truncate;
    self->Base.Exists = FatFsFile_Exists;
    self->Base.Delete = FatFsFile_Delete;
    self->IsOpen = false;
}

static inline struct SolidSyslogFatFsFile* FatFsFile_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogFatFsFile*) base;
}

void FatFsFile_Cleanup(struct SolidSyslogFile* base)
{
    FatFsFile_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
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

static bool FatFsFile_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogFatFsFile* self = FatFsFile_SelfFromBase(base);
    FRESULT result = f_open(&self->Fp, path, READ_WRITE_OR_CREATE);
    self->IsOpen = (result == FR_OK);
    return self->IsOpen;
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
