#include "SolidSyslogNullFile.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogFileDefinition.h"

static bool NullFile_Open(struct SolidSyslogFile* base, const char* path);
static void NullFile_Close(struct SolidSyslogFile* base);
static bool NullFile_IsOpen(struct SolidSyslogFile* base);
static bool NullFile_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool NullFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void NullFile_SeekTo(struct SolidSyslogFile* base, size_t offset);
static size_t NullFile_Size(struct SolidSyslogFile* base);
static void NullFile_Truncate(struct SolidSyslogFile* base);
static bool NullFile_Exists(struct SolidSyslogFile* base, const char* path);
static bool NullFile_Delete(struct SolidSyslogFile* base, const char* path);

struct SolidSyslogFile* SolidSyslogNullFile_Get(void)
{
    static struct SolidSyslogFile instance = {
        .Open = NullFile_Open,
        .Close = NullFile_Close,
        .IsOpen = NullFile_IsOpen,
        .Read = NullFile_Read,
        .Write = NullFile_Write,
        .SeekTo = NullFile_SeekTo,
        .Size = NullFile_Size,
        .Truncate = NullFile_Truncate,
        .Exists = NullFile_Exists,
        .Delete = NullFile_Delete,
    };
    return &instance;
}

/* Open returns false so callers see a consistently non-functional file.
 * NullFile is the fallback when PosixFile / WindowsFile / FatFsFile
 * Create exhausts the pool — at that point the wider chain
 * (BlockStore → NullStore) is already broken; presenting "open failed"
 * lets the consumer's existing error path handle it cleanly. */
static bool NullFile_Open(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    (void) path;
    return false;
}

static void NullFile_Close(struct SolidSyslogFile* base)
{
    (void) base;
}

static bool NullFile_IsOpen(struct SolidSyslogFile* base)
{
    (void) base;
    return false;
}

static bool NullFile_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    (void) base;
    (void) buf;
    (void) count;
    return false;
}

/* Write returns true so the BlockStore-side caller treats the bytes as
 * persisted and does not retry. Mirrors NullSender_Send's drop-on-the-floor. */
static bool NullFile_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    (void) base;
    (void) buf;
    (void) count;
    return true;
}

static void NullFile_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    (void) base;
    (void) offset;
}

static size_t NullFile_Size(struct SolidSyslogFile* base)
{
    (void) base;
    return 0U;
}

static void NullFile_Truncate(struct SolidSyslogFile* base)
{
    (void) base;
}

static bool NullFile_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    (void) path;
    return false;
}

/* Delete returns true ("succeeded vacuously") so callers that treat
 * delete-of-nonexistent as a no-op are not tripped by the null object. */
static bool NullFile_Delete(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    (void) path;
    return true;
}
