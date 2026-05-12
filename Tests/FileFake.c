#include "FileFake.h"

#include <string.h>
#include <stdbool.h>

#include "SafeString.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogTunables.h"
#include "TestAssert.h"

enum
{
    /* Per-file storage. Sized to comfortably hold the worst-case test
     * scenario, which writes up to two SOLIDSYSLOG_MAX_MESSAGE_SIZE
     * records per file plus record metadata and integrity bytes. The
     * 4x multiplier provides headroom for tests that pre-seed file
     * content before opening the store. Auto-adapts when
     * SOLIDSYSLOG_MAX_MESSAGE_SIZE is tuned. */
    FILEFAKE_MAX_SIZE  = 4 * SOLIDSYSLOG_MAX_MESSAGE_SIZE,
    FILEFAKE_MAX_FILES = 101,
    FILEFAKE_MAX_PATH  = 128
};

static bool   FileFake_Open(struct SolidSyslogFile* self, const char* path);
static void   FileFake_Close(struct SolidSyslogFile* self);
static bool   FileFake_IsOpen(struct SolidSyslogFile* self);
static bool   FileFake_Read(struct SolidSyslogFile* self, void* buf, size_t count);
static bool   FileFake_Write(struct SolidSyslogFile* self, const void* buf, size_t count);
static void   FileFake_SeekTo(struct SolidSyslogFile* self, size_t offset);
static size_t FileFake_Size(struct SolidSyslogFile* self);
static void   FileFake_Truncate(struct SolidSyslogFile* self);
static bool   FileFake_Exists(struct SolidSyslogFile* self, const char* path);
static bool   FileFake_Delete(struct SolidSyslogFile* self, const char* path);

/* poisoned vtable — installed by Destroy to catch use-after-destroy */
static bool   FileFake_DestroyedOpen(struct SolidSyslogFile* self, const char* path);
static void   FileFake_DestroyedClose(struct SolidSyslogFile* self);
static bool   FileFake_DestroyedIsOpen(struct SolidSyslogFile* self);
static bool   FileFake_DestroyedRead(struct SolidSyslogFile* self, void* buf, size_t count);
static bool   FileFake_DestroyedWrite(struct SolidSyslogFile* self, const void* buf, size_t count);
static void   FileFake_DestroyedSeekTo(struct SolidSyslogFile* self, size_t offset);
static size_t FileFake_DestroyedSize(struct SolidSyslogFile* self);
static void   FileFake_DestroyedTruncate(struct SolidSyslogFile* self);
static bool   FileFake_DestroyedExists(struct SolidSyslogFile* self, const char* path);
static bool   FileFake_DestroyedDelete(struct SolidSyslogFile* self, const char* path);

struct FileEntry
{
    char   path[FILEFAKE_MAX_PATH];
    char   content[FILEFAKE_MAX_SIZE];
    size_t fileSize;
    bool   inUse;
};

struct FileFake
{
    struct SolidSyslogFile base;
    struct FileEntry*      active;
    size_t                 position;
    bool                   open;
    bool                   failNextOpen;
    bool                   failNextWrite;
    bool                   failNextRead;
    bool                   failNextDelete;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct FileFake) <= sizeof(struct FileFakeStorage), "FileFakeStorage is too small for struct FileFake");

/* shared in-memory filesystem */
static struct FileEntry filesystem[FILEFAKE_MAX_FILES];

/* pointer to most recently created instance — used by FailNext* and inspection helpers */
static struct FileFake* lastCreated;

/* helpers */
static inline struct FileFake* AsFake(struct SolidSyslogFile* self);
static void                    RequireOpenFile(struct FileFake* fake, const char* message);
static inline bool             IsFileClosed(const struct FileFake* fake);
static inline bool             HasActiveFile(const struct FileFake* fake);
static inline bool             ShouldFailOnThisCall(bool* flag);
static inline bool             FoundEntry(const struct FileEntry* entry);
static inline void             ActivateEntry(struct FileFake* fake, struct FileEntry* entry);
static inline bool             HasBytesToRead(const struct FileFake* fake, size_t count);
static inline void             CopyFromFile(struct FileFake* fake, void* buf, size_t count);
static inline bool             HasSpaceToWrite(const struct FileFake* fake, size_t count);
static inline void             CopyToFile(struct FileFake* fake, const void* buf, size_t count);
static inline void             AdvancePosition(struct FileFake* fake, size_t count);
static inline void             ExtendFileSize(struct FileFake* fake);
static inline void             ResetActiveFile(struct FileFake* fake);
static struct FileEntry*       FindEntry(const char* path);
static struct FileEntry*       FindOrCreateEntry(const char* path);
static struct FileEntry*       FindFreeSlot(void);
static inline bool             IsSlotFree(const struct FileEntry* entry);
static inline bool             EntryMatchesPath(const struct FileEntry* entry, const char* path);
static inline void             InitialiseEntry(struct FileEntry* entry, const char* path);
static inline void             ClearEntry(struct FileEntry* entry);

static const struct SolidSyslogFile LIVE_VTABLE = {
    FileFake_Open,   FileFake_Close, FileFake_IsOpen,   FileFake_Read,   FileFake_Write,
    FileFake_SeekTo, FileFake_Size,  FileFake_Truncate, FileFake_Exists, FileFake_Delete,
};

static const struct SolidSyslogFile POISONED_VTABLE = {
    FileFake_DestroyedOpen,   FileFake_DestroyedClose, FileFake_DestroyedIsOpen,   FileFake_DestroyedRead,   FileFake_DestroyedWrite,
    FileFake_DestroyedSeekTo, FileFake_DestroyedSize,  FileFake_DestroyedTruncate, FileFake_DestroyedExists, FileFake_DestroyedDelete,
};

/* ------------------------------------------------------------------
 * Create / Destroy
 * ----------------------------------------------------------------*/

struct SolidSyslogFile* FileFake_Create(struct FileFakeStorage* storage)
{
    struct FileFake* fake = (struct FileFake*) storage;
    memset(fake, 0, sizeof(*fake));
    fake->base  = LIVE_VTABLE;
    lastCreated = fake;
    return &fake->base;
}

void FileFake_Destroy(void)
{
    if (lastCreated != NULL)
    {
        memset(lastCreated, 0, sizeof(*lastCreated));
        lastCreated->base = POISONED_VTABLE;
        lastCreated       = NULL;
    }

    memset(filesystem, 0, sizeof(filesystem));
}

/* ------------------------------------------------------------------
 * Fail injection
 * ----------------------------------------------------------------*/

void FileFake_FailNextOpen(struct SolidSyslogFile* file)
{
    if (file == NULL)
    {
        TestAssert_Fail("FileFake_FailNextOpen called with null file");
        return;
    }
    AsFake(file)->failNextOpen = true;
}

void FileFake_FailNextWrite(struct SolidSyslogFile* file)
{
    if (file == NULL)
    {
        TestAssert_Fail("FileFake_FailNextWrite called with null file");
        return;
    }
    AsFake(file)->failNextWrite = true;
}

void FileFake_FailNextRead(struct SolidSyslogFile* file)
{
    if (file == NULL)
    {
        TestAssert_Fail("FileFake_FailNextRead called with null file");
        return;
    }
    AsFake(file)->failNextRead = true;
}

void FileFake_FailNextDelete(struct SolidSyslogFile* file)
{
    if (file == NULL)
    {
        TestAssert_Fail("FileFake_FailNextDelete called with null file");
        return;
    }
    AsFake(file)->failNextDelete = true;
}

/* ------------------------------------------------------------------
 * Inspection
 * ----------------------------------------------------------------*/

const void* FileFake_FileContent(void)
{
    return HasActiveFile(lastCreated) ? lastCreated->active->content : NULL;
}

size_t FileFake_FileSize(void)
{
    return HasActiveFile(lastCreated) ? lastCreated->active->fileSize : 0;
}

static inline bool HasActiveFile(const struct FileFake* fake)
{
    return FoundEntry(fake->active);
}

/* ------------------------------------------------------------------
 * Open
 * ----------------------------------------------------------------*/

static bool FileFake_Open(struct SolidSyslogFile* self, const char* path)
{
    struct FileFake* fake = AsFake(self);

    if (ShouldFailOnThisCall(&fake->failNextOpen))
    {
        return false;
    }

    struct FileEntry* entry = FindOrCreateEntry(path);

    if (FoundEntry(entry))
    {
        ActivateEntry(fake, entry);
    }

    return FoundEntry(entry);
}

static inline struct FileFake* AsFake(struct SolidSyslogFile* self)
{
    return (struct FileFake*) self;
}

static inline bool ShouldFailOnThisCall(bool* flag)
{
    bool consumed = *flag;
    *flag         = false;
    return consumed;
}

static inline bool FoundEntry(const struct FileEntry* entry)
{
    return entry != NULL;
}

static inline void ActivateEntry(struct FileFake* fake, struct FileEntry* entry)
{
    fake->active   = entry;
    fake->open     = true;
    fake->position = 0;
}

static struct FileEntry* FindOrCreateEntry(const char* path)
{
    struct FileEntry* entry = FindEntry(path);

    if (entry == NULL)
    {
        entry = FindFreeSlot();

        if (FoundEntry(entry))
        {
            InitialiseEntry(entry, path);
        }
    }

    return entry;
}

static struct FileEntry* FindEntry(const char* path)
{
    struct FileEntry* result = NULL;

    for (size_t i = 0; i < FILEFAKE_MAX_FILES; i++)
    {
        if (EntryMatchesPath(&filesystem[i], path))
        {
            result = &filesystem[i];
            break;
        }
    }

    return result;
}

static inline bool EntryMatchesPath(const struct FileEntry* entry, const char* path)
{
    return entry->inUse && (strcmp(entry->path, path) == 0);
}

static struct FileEntry* FindFreeSlot(void)
{
    struct FileEntry* result = NULL;

    for (size_t i = 0; i < FILEFAKE_MAX_FILES; i++)
    {
        if (IsSlotFree(&filesystem[i]))
        {
            result = &filesystem[i];
            break;
        }
    }

    return result;
}

static inline bool IsSlotFree(const struct FileEntry* entry)
{
    return !entry->inUse;
}

static inline void InitialiseEntry(struct FileEntry* entry, const char* path)
{
    entry->inUse = true;
    SafeString_Copy(entry->path, FILEFAKE_MAX_PATH, path);
}

/* ------------------------------------------------------------------
 * Close
 * ----------------------------------------------------------------*/

static void FileFake_Close(struct SolidSyslogFile* self)
{
    struct FileFake* fake = AsFake(self);
    RequireOpenFile(fake, "Close called with no file open");
    fake->open     = false;
    fake->position = 0;
}

static void RequireOpenFile(struct FileFake* fake, const char* message)
{
    if (IsFileClosed(fake))
    {
        TestAssert_Fail(message);
    }
}

static inline bool IsFileClosed(const struct FileFake* fake)
{
    return !fake->open;
}

/* ------------------------------------------------------------------
 * IsOpen
 * ----------------------------------------------------------------*/

static bool FileFake_IsOpen(struct SolidSyslogFile* self)
{
    struct FileFake* fake = AsFake(self);
    return fake->open;
}

/* ------------------------------------------------------------------
 * Read
 * ----------------------------------------------------------------*/

static bool FileFake_Read(struct SolidSyslogFile* self, void* buf, size_t count)
{
    struct FileFake* fake = AsFake(self);
    RequireOpenFile(fake, "Read called with no file open");

    if (ShouldFailOnThisCall(&fake->failNextRead))
    {
        return false;
    }

    bool success = HasBytesToRead(fake, count);

    if (success)
    {
        CopyFromFile(fake, buf, count);
    }

    return success;
}

static inline bool HasBytesToRead(const struct FileFake* fake, size_t count)
{
    return (fake->position + count) <= fake->active->fileSize;
}

static inline void CopyFromFile(struct FileFake* fake, void* buf, size_t count)
{
    memcpy(buf, fake->active->content + fake->position, count);
    AdvancePosition(fake, count);
}

static inline void AdvancePosition(struct FileFake* fake, size_t count)
{
    fake->position += count;
}

/* ------------------------------------------------------------------
 * Write
 * ----------------------------------------------------------------*/

static bool FileFake_Write(struct SolidSyslogFile* self, const void* buf, size_t count)
{
    struct FileFake* fake = AsFake(self);
    RequireOpenFile(fake, "Write called with no file open");

    if (ShouldFailOnThisCall(&fake->failNextWrite))
    {
        return false;
    }

    bool success = HasSpaceToWrite(fake, count);

    if (success)
    {
        CopyToFile(fake, buf, count);
    }

    return success;
}

static inline bool HasSpaceToWrite(const struct FileFake* fake, size_t count)
{
    return (fake->position + count) <= FILEFAKE_MAX_SIZE;
}

static inline void CopyToFile(struct FileFake* fake, const void* buf, size_t count)
{
    memcpy(fake->active->content + fake->position, buf, count);
    AdvancePosition(fake, count);
    ExtendFileSize(fake);
}

static inline void ExtendFileSize(struct FileFake* fake)
{
    if (fake->position > fake->active->fileSize)
    {
        fake->active->fileSize = fake->position;
    }
}

/* ------------------------------------------------------------------
 * SeekTo / Size / Truncate / Exists
 * ----------------------------------------------------------------*/

static void FileFake_SeekTo(struct SolidSyslogFile* self, size_t offset)
{
    struct FileFake* fake = AsFake(self);
    RequireOpenFile(fake, "SeekTo called with no file open");
    fake->position = offset;
}

static size_t FileFake_Size(struct SolidSyslogFile* self)
{
    struct FileFake* fake = AsFake(self);
    RequireOpenFile(fake, "Size called with no file open");
    return fake->active->fileSize;
}

static void FileFake_Truncate(struct SolidSyslogFile* self)
{
    struct FileFake* fake = AsFake(self);
    RequireOpenFile(fake, "Truncate called with no file open");
    ResetActiveFile(fake);
}

static inline void ResetActiveFile(struct FileFake* fake)
{
    fake->active->fileSize = 0;
    fake->position         = 0;
}

static bool FileFake_Exists(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    return FoundEntry(FindEntry(path));
}

static bool FileFake_Delete(struct SolidSyslogFile* self, const char* path)
{
    struct FileFake* fake = AsFake(self);

    if (ShouldFailOnThisCall(&fake->failNextDelete))
    {
        return false;
    }

    struct FileEntry* entry = FindEntry(path);
    bool              found = FoundEntry(entry);

    if (found)
    {
        ClearEntry(entry);
    }

    return found;
}

static inline void ClearEntry(struct FileEntry* entry)
{
    memset(entry->content, 0, sizeof(entry->content));
    entry->fileSize = 0;
    entry->path[0]  = '\0';
    /* inUse stays true — prevents slot reuse while stale handles may reference this entry */
}

/* ------------------------------------------------------------------
 * Poisoned vtable — installed by Destroy
 * ----------------------------------------------------------------*/

static bool FileFake_DestroyedOpen(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    (void) path;
    TestAssert_Fail("Open called after FileFake_Destroy");
    return false;
}

static void FileFake_DestroyedClose(struct SolidSyslogFile* self)
{
    (void) self;
    TestAssert_Fail("Close called after FileFake_Destroy");
}

static bool FileFake_DestroyedIsOpen(struct SolidSyslogFile* self)
{
    (void) self;
    TestAssert_Fail("IsOpen called after FileFake_Destroy");
    return false;
}

static bool FileFake_DestroyedRead(struct SolidSyslogFile* self, void* buf, size_t count)
{
    (void) self;
    (void) buf;
    (void) count;
    TestAssert_Fail("Read called after FileFake_Destroy");
    return false;
}

static bool FileFake_DestroyedWrite(struct SolidSyslogFile* self, const void* buf, size_t count)
{
    (void) self;
    (void) buf;
    (void) count;
    TestAssert_Fail("Write called after FileFake_Destroy");
    return false;
}

static void FileFake_DestroyedSeekTo(struct SolidSyslogFile* self, size_t offset)
{
    (void) self;
    (void) offset;
    TestAssert_Fail("SeekTo called after FileFake_Destroy");
}

static size_t FileFake_DestroyedSize(struct SolidSyslogFile* self)
{
    (void) self;
    TestAssert_Fail("Size called after FileFake_Destroy");
    return 0;
}

static void FileFake_DestroyedTruncate(struct SolidSyslogFile* self)
{
    (void) self;
    TestAssert_Fail("Truncate called after FileFake_Destroy");
}

static bool FileFake_DestroyedExists(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    (void) path;
    TestAssert_Fail("Exists called after FileFake_Destroy");
    return false;
}

static bool FileFake_DestroyedDelete(struct SolidSyslogFile* self, const char* path)
{
    (void) self;
    (void) path;
    TestAssert_Fail("Delete called after FileFake_Destroy");
    return false;
}
