#include "SolidSyslogFileStore.h"
#include "BlockSequence.h"
#include "RecordStore.h"
#include "SolidSyslog.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogStoreDefinition.h"

/* vtable — forward-declared because InitialiseVtable references them before their definitions */
static bool   Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool   ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void   MarkSent(struct SolidSyslogStore* self);
static bool   HasUnsent(struct SolidSyslogStore* self);
static bool   IsHalted(struct SolidSyslogStore* self);
static size_t GetTotalBytes(struct SolidSyslogStore* self);
static size_t GetUsedBytes(struct SolidSyslogStore* self);

/* ------------------------------------------------------------------
 * Instance
 * ----------------------------------------------------------------*/

struct SolidSyslogFileStore
{
    struct SolidSyslogStore base;
    struct RecordStore      recordStore;
    struct BlockSequence    blockSequence;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogFileStore) <= sizeof(SolidSyslogFileStoreStorage),
                          "SOLIDSYSLOG_FILESTORE_STORAGE_SIZE is too small for struct SolidSyslogFileStore");

static const struct SolidSyslogFileStore DEFAULT_INSTANCE = {0};

static inline struct SolidSyslogFileStore*      AsFileStore(struct SolidSyslogStore* store);
static inline struct SolidSyslogSecurityPolicy* ResolveSecurityPolicy(struct SolidSyslogSecurityPolicy* configured);
static inline struct BlockSequenceConfig BuildBlockSequenceConfig(const struct SolidSyslogFileStoreConfig* config, const struct RecordStore* recordStore);
static inline void                       InitialiseVtable(struct SolidSyslogFileStore* fileStore);
static void                              ResumeFromExistingFile(struct SolidSyslogFileStore* fileStore);

/* ------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------*/

struct SolidSyslogStore* SolidSyslogFileStore_Create(SolidSyslogFileStoreStorage* storage, const struct SolidSyslogFileStoreConfig* config)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) -- C header; integrator-supplied storage blob recast to impl
    struct SolidSyslogFileStore* fileStore = (struct SolidSyslogFileStore*) storage;
    *fileStore                             = DEFAULT_INSTANCE;

    RecordStore_Init(&fileStore->recordStore, ResolveSecurityPolicy(config->securityPolicy));

    struct BlockSequenceConfig blockConfig = BuildBlockSequenceConfig(config, &fileStore->recordStore);
    BlockSequence_Init(&fileStore->blockSequence, &blockConfig);

    InitialiseVtable(fileStore);

    if (BlockSequence_Open(&fileStore->blockSequence))
    {
        ResumeFromExistingFile(fileStore);
    }

    return &fileStore->base;
}

static inline struct SolidSyslogFileStore* AsFileStore(struct SolidSyslogStore* store)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) -- C; base is the first member of struct SolidSyslogFileStore
    return (struct SolidSyslogFileStore*) store;
}

static inline struct SolidSyslogSecurityPolicy* ResolveSecurityPolicy(struct SolidSyslogSecurityPolicy* configured)
{
    struct SolidSyslogSecurityPolicy* resolved = configured;

    if ((resolved == NULL) || (resolved->integritySize > SOLIDSYSLOG_MAX_INTEGRITY_SIZE))
    {
        resolved = SolidSyslogNullSecurityPolicy_Create();
    }

    return resolved;
}

static inline struct BlockSequenceConfig BuildBlockSequenceConfig(const struct SolidSyslogFileStoreConfig* config, const struct RecordStore* recordStore)
{
    size_t minFileSize = RecordStore_RecordSize(recordStore, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
    size_t maxFileSize = (config->maxFileSize < minFileSize) ? minFileSize : config->maxFileSize;

    struct BlockSequenceConfig blockConfig = {
        .readFile         = config->readFile,
        .writeFile        = config->writeFile,
        .pathPrefix       = config->pathPrefix,
        .maxFileSize      = maxFileSize,
        .maxFiles         = config->maxFiles,
        .discardPolicy    = config->discardPolicy,
        .onStoreFull      = config->onStoreFull,
        .storeFullContext = config->storeFullContext,
    };
    return blockConfig;
}

static inline void InitialiseVtable(struct SolidSyslogFileStore* fileStore)
{
    fileStore->base.Write          = Write;
    fileStore->base.ReadNextUnsent = ReadNextUnsent;
    fileStore->base.MarkSent       = MarkSent;
    fileStore->base.HasUnsent      = HasUnsent;
    fileStore->base.IsHalted       = IsHalted;
    fileStore->base.GetTotalBytes  = GetTotalBytes;
    fileStore->base.GetUsedBytes   = GetUsedBytes;
}

static void ResumeFromExistingFile(struct SolidSyslogFileStore* fileStore)
{
    bool   corrupt = false;
    size_t cursor  = RecordStore_FindFirstUnsent(&fileStore->recordStore, BlockSequence_ReadFile(&fileStore->blockSequence),
                                                 BlockSequence_WritePosition(&fileStore->blockSequence), &corrupt);

    BlockSequence_SetReadCursor(&fileStore->blockSequence, cursor);

    if (corrupt)
    {
        BlockSequence_MarkWriteFileCorrupt(&fileStore->blockSequence);
    }
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

void SolidSyslogFileStore_Destroy(struct SolidSyslogStore* store)
{
    struct SolidSyslogFileStore* fileStore = AsFileStore(store);
    BlockSequence_Close(&fileStore->blockSequence);
    *fileStore = DEFAULT_INSTANCE;
}

/* ------------------------------------------------------------------
 * Write
 * ----------------------------------------------------------------*/

static bool StoreRecord(struct SolidSyslogFileStore* fileStore, const void* data, size_t size);

static bool Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    struct SolidSyslogFileStore* fileStore = AsFileStore(self);
    bool                         written   = false;

    if (SolidSyslogFile_IsOpen(BlockSequence_WriteFile(&fileStore->blockSequence)))
    {
        written = StoreRecord(fileStore, data, size);
    }

    return written;
}

static bool StoreRecord(struct SolidSyslogFileStore* fileStore, const void* data, size_t size)
{
    size_t recordSize      = RecordStore_RecordSize(&fileStore->recordStore, (uint16_t) size);
    bool   readFileChanged = false;
    bool   written         = false;

    if (BlockSequence_PrepareForWrite(&fileStore->blockSequence, recordSize, &readFileChanged))
    {
        if (readFileChanged)
        {
            RecordStore_ForgetLastRead(&fileStore->recordStore);
        }

        if (RecordStore_Append(&fileStore->recordStore, BlockSequence_WriteFile(&fileStore->blockSequence),
                               BlockSequence_WritePosition(&fileStore->blockSequence), data, size))
        {
            BlockSequence_NoteRecordWritten(&fileStore->blockSequence, recordSize);
            written = true;
        }
    }

    return written;
}

/* ------------------------------------------------------------------
 * HasUnsent / IsHalted
 * ----------------------------------------------------------------*/

static bool HasUnsent(struct SolidSyslogStore* self)
{
    return BlockSequence_HasUnsent(&AsFileStore(self)->blockSequence);
}

static bool IsHalted(struct SolidSyslogStore* self)
{
    return BlockSequence_IsHalted(&AsFileStore(self)->blockSequence);
}

static size_t GetTotalBytes(struct SolidSyslogStore* self)
{
    return BlockSequence_TotalBytes(&AsFileStore(self)->blockSequence);
}

static size_t GetUsedBytes(struct SolidSyslogStore* self)
{
    return BlockSequence_UsedBytes(&AsFileStore(self)->blockSequence);
}

/* ------------------------------------------------------------------
 * ReadNextUnsent
 * ----------------------------------------------------------------*/

static bool ReadCurrent(struct SolidSyslogFileStore* fileStore, void* data, size_t maxSize, size_t* bytesRead);

static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogFileStore* fileStore = AsFileStore(self);
    bool                         read      = false;
    *bytesRead                             = 0;

    if (BlockSequence_HasUnsent(&fileStore->blockSequence))
    {
        read = ReadCurrent(fileStore, data, maxSize, bytesRead);

        while (!read && BlockSequence_IsReadingOlderFile(&fileStore->blockSequence))
        {
            BlockSequence_AdvanceToNextReadFile(&fileStore->blockSequence);
            RecordStore_ForgetLastRead(&fileStore->recordStore);
            read = ReadCurrent(fileStore, data, maxSize, bytesRead);
        }
    }

    return read;
}

static bool ReadCurrent(struct SolidSyslogFileStore* fileStore, void* data, size_t maxSize, size_t* bytesRead)
{
    return RecordStore_Read(&fileStore->recordStore, BlockSequence_ReadFile(&fileStore->blockSequence), BlockSequence_ReadCursor(&fileStore->blockSequence),
                            data, maxSize, bytesRead);
}

/* ------------------------------------------------------------------
 * MarkSent
 * ----------------------------------------------------------------*/

static void MarkSent(struct SolidSyslogStore* self)
{
    struct SolidSyslogFileStore* fileStore  = AsFileStore(self);
    size_t                       nextCursor = 0;

    if (RecordStore_MarkLastReadAsSent(&fileStore->recordStore, BlockSequence_ReadFile(&fileStore->blockSequence), &nextCursor))
    {
        BlockSequence_SetReadCursor(&fileStore->blockSequence, nextCursor);
    }
}
