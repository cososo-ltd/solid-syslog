#include "TestCalloc.h"
#include "RamFileFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogFileErrors.h"

enum
{
    RAMFILEFAKE_MAX_FILES = 8,
    RAMFILEFAKE_MAX_PATH = 64
};

struct RamFileEntry
{
    char Path[RAMFILEFAKE_MAX_PATH];
    uint8_t* Bytes;
    size_t Size;
    bool Used;
};

struct RamFileFake
{
    struct SolidSyslogFile Base;
    struct RamFileEntry Files[RAMFILEFAKE_MAX_FILES];
    struct RamFileEntry* OpenFile;
    size_t Position;
    bool Failed;
    int32_t NativeErrno;
};

const struct SolidSyslogErrorSource RamFileFakeErrorSource = {"RamFileFake", SolidSyslogFileErrors_Name};

static bool Open(struct SolidSyslogFile* base, const char* path);
static void Close(struct SolidSyslogFile* base);
static bool IsOpen(struct SolidSyslogFile* base);
static bool Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static bool Grow(struct RamFileEntry* entry, size_t size);
static void SeekTo(struct SolidSyslogFile* base, size_t offset);
static bool Size(struct SolidSyslogFile* base, size_t* size);
static void Truncate(struct SolidSyslogFile* base);
static bool Exists(struct SolidSyslogFile* base, const char* path);
static bool Delete(struct SolidSyslogFile* base, const char* path);
static struct RamFileEntry* Find(struct RamFileFake* fake, const char* path);
static struct RamFileEntry* Add(struct RamFileFake* fake, const char* path);
static bool FailsAs(const struct RamFileFake* fake, enum SolidSyslogFileErrors code);

struct SolidSyslogFile* RamFileFake_Create(void)
{
    struct RamFileFake* fake = (struct RamFileFake*) TestCalloc(sizeof(struct RamFileFake));
    fake->Base.Open = Open;
    fake->Base.Close = Close;
    fake->Base.IsOpen = IsOpen;
    fake->Base.Read = Read;
    fake->Base.Write = Write;
    fake->Base.SeekTo = SeekTo;
    fake->Base.Size = Size;
    fake->Base.Truncate = Truncate;
    fake->Base.Exists = Exists;
    fake->Base.Delete = Delete;
    return &fake->Base;
}

void RamFileFake_Destroy(struct SolidSyslogFile* file)
{
    struct RamFileFake* fake = (struct RamFileFake*) file;
    for (size_t i = 0U; i < RAMFILEFAKE_MAX_FILES; i++)
    {
        free(fake->Files[i].Bytes);
    }
    free(fake);
}

void RamFileFake_FailEveryOperation(struct SolidSyslogFile* file, int32_t nativeErrno)
{
    struct RamFileFake* fake = (struct RamFileFake*) file;
    fake->Failed = true;
    fake->NativeErrno = nativeErrno;
}

static bool Open(struct SolidSyslogFile* base, const char* path)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;
    bool opened = !FailsAs(fake, SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED);

    if (opened)
    {
        struct RamFileEntry* entry = Find(fake, path);
        fake->OpenFile = (entry != NULL) ? entry : Add(fake, path);
        fake->Position = 0U;
        opened = fake->OpenFile != NULL;
    }

    return opened;
}

static void Close(struct SolidSyslogFile* base)
{
    ((struct RamFileFake*) base)->OpenFile = NULL;
}

static bool IsOpen(struct SolidSyslogFile* base)
{
    return ((struct RamFileFake*) base)->OpenFile != NULL;
}

static bool Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;
    bool whole = !FailsAs(fake, SOLIDSYSLOG_FILE_ERROR_READ_FAILED) && (fake->OpenFile != NULL) &&
                 ((fake->Position + count) <= fake->OpenFile->Size);

    if (whole)
    {
        memcpy(buf, &fake->OpenFile->Bytes[fake->Position], count);
        fake->Position += count;
    }

    return whole;
}

/* A write past the end extends the file, filling any gap with zeros. */
static bool Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;
    bool written = !FailsAs(fake, SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED) && (fake->OpenFile != NULL);

    if (written)
    {
        struct RamFileEntry* entry = fake->OpenFile;
        size_t end = fake->Position + count;
        written = (end <= entry->Size) || Grow(entry, end);
        if (written)
        {
            memcpy(&entry->Bytes[fake->Position], buf, count);
            fake->Position = end;
        }
    }

    return written;
}

/* Extends the file to @p size bytes, the new ones zero. False, leaving it as it
 * was, when the memory cannot be had. */
static bool Grow(struct RamFileEntry* entry, size_t size)
{
    uint8_t* grown = (uint8_t*) realloc(entry->Bytes, size);
    if (grown != NULL)
    {
        memset(&grown[entry->Size], 0, size - entry->Size);
        entry->Bytes = grown;
        entry->Size = size;
    }
    return grown != NULL;
}

static void SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;

    if (!FailsAs(fake, SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED))
    {
        fake->Position = offset;
    }
}

static bool Size(struct SolidSyslogFile* base, size_t* size)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;
    bool known = !FailsAs(fake, SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED) && (fake->OpenFile != NULL);
    *size = known ? fake->OpenFile->Size : 0U;
    return known;
}

static void Truncate(struct SolidSyslogFile* base)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;

    if (!FailsAs(fake, SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED) && (fake->OpenFile != NULL))
    {
        fake->OpenFile->Size = 0U;
        fake->Position = 0U;
    }
}

static bool Exists(struct SolidSyslogFile* base, const char* path)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;
    return !fake->Failed && (Find(fake, path) != NULL);
}

/* An absent path counts as removed, as the contract says. */
static bool Delete(struct SolidSyslogFile* base, const char* path)
{
    struct RamFileFake* fake = (struct RamFileFake*) base;
    struct RamFileEntry* entry = fake->Failed ? NULL : Find(fake, path);

    if (entry != NULL)
    {
        free(entry->Bytes);
        memset(entry, 0, sizeof(*entry));
    }

    return !fake->Failed;
}

static struct RamFileEntry* Find(struct RamFileFake* fake, const char* path)
{
    struct RamFileEntry* found = NULL;
    for (size_t i = 0U; i < RAMFILEFAKE_MAX_FILES; i++)
    {
        if (fake->Files[i].Used && (strcmp(fake->Files[i].Path, path) == 0))
        {
            found = &fake->Files[i];
        }
    }
    return found;
}

/* A new, empty file at @p path; NULL when the fake holds as many as it can or
 * the path is too long to keep. */
static struct RamFileEntry* Add(struct RamFileFake* fake, const char* path)
{
    size_t length = strlen(path);
    struct RamFileEntry* added = NULL;
    for (size_t i = 0U; (added == NULL) && (i < RAMFILEFAKE_MAX_FILES) && (length < RAMFILEFAKE_MAX_PATH); i++)
    {
        if (!fake->Files[i].Used)
        {
            added = &fake->Files[i];
            memcpy(added->Path, path, length + 1U);
            added->Used = true;
        }
    }
    return added;
}

/* A failed file reports the operation it could not do, with its errno. */
static bool FailsAs(const struct RamFileFake* fake, enum SolidSyslogFileErrors code)
{
    if (fake->Failed)
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
            &RamFileFakeErrorSource,
            SOLIDSYSLOG_CAT_FILE_IO_FAILED,
            (int32_t) code,
            fake->NativeErrno
        );
    }
    return fake->Failed;
}
