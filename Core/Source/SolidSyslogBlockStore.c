#include "SolidSyslogBlockStore.h"

#include <stdbool.h>

#include "BlockSequence.h"
#include "RecordStore.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogStoreDefinition.h"

/* vtable — forward-declared because BlockStore_InitialiseVtable references them before their definitions */
static bool BlockStore_Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void BlockStore_MarkSent(struct SolidSyslogStore* self);
static bool BlockStore_HasUnsent(struct SolidSyslogStore* self);
static bool BlockStore_IsHalted(struct SolidSyslogStore* self);
static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* self);
static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* self);
static bool BlockStore_IsTransient(struct SolidSyslogStore* self);

/* ------------------------------------------------------------------
 * Instance
 * ----------------------------------------------------------------*/

struct SolidSyslogBlockStore
{
    struct SolidSyslogStore Base;
    struct RecordStore RecordStore;
    struct BlockSequence BlockSequence;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogBlockStore) <= sizeof(SolidSyslogBlockStoreStorage),
    "SOLIDSYSLOG_BLOCKSTORE_STORAGE_SIZE is too small for struct SolidSyslogBlockStore"
);

static const struct SolidSyslogBlockStore DEFAULT_INSTANCE = {0};

static inline struct SolidSyslogBlockStore* BlockStore_AsBlockStore(struct SolidSyslogStore* store);
static inline struct SolidSyslogSecurityPolicy* BlockStore_ResolveSecurityPolicy(
    struct SolidSyslogSecurityPolicy* configured
);
static inline struct BlockSequenceConfig BlockStore_BuildBlockSequenceConfig(
    const struct SolidSyslogBlockStoreConfig* config,
    const struct RecordStore* recordStore
);
static inline void BlockStore_InitialiseVtable(struct SolidSyslogBlockStore* blockStore);
static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* blockStore);

/* ------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------*/

struct SolidSyslogStore* SolidSyslogBlockStore_Create(
    SolidSyslogBlockStoreStorage* storage,
    const struct SolidSyslogBlockStoreConfig* config
)
{
    struct SolidSyslogBlockStore* blockStore = (struct SolidSyslogBlockStore*) storage;
    *blockStore = DEFAULT_INSTANCE;

    RecordStore_Init(&blockStore->RecordStore, BlockStore_ResolveSecurityPolicy(config->SecurityPolicy));

    struct BlockSequenceConfig blockConfig = BlockStore_BuildBlockSequenceConfig(config, &blockStore->RecordStore);
    BlockSequence_Init(&blockStore->BlockSequence, &blockConfig);

    BlockStore_InitialiseVtable(blockStore);

    if (BlockSequence_Open(&blockStore->BlockSequence))
    {
        BlockStore_ResumeFromExistingBlock(blockStore);
    }

    return &blockStore->Base;
}

static inline struct SolidSyslogBlockStore* BlockStore_AsBlockStore(struct SolidSyslogStore* store)
{
    return (struct SolidSyslogBlockStore*) store;
}

static inline struct SolidSyslogSecurityPolicy* BlockStore_ResolveSecurityPolicy(
    struct SolidSyslogSecurityPolicy* configured
)
{
    struct SolidSyslogSecurityPolicy* resolved = configured;

    if ((resolved == NULL) || (resolved->IntegritySize > SOLIDSYSLOG_MAX_INTEGRITY_SIZE))
    {
        resolved = SolidSyslogNullSecurityPolicy_Create();
    }

    return resolved;
}

static inline struct BlockSequenceConfig BlockStore_BuildBlockSequenceConfig(
    const struct SolidSyslogBlockStoreConfig* config,
    const struct RecordStore* recordStore
)
{
    size_t minBlockSize = RecordStore_RecordSize(recordStore, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
    size_t maxBlockSize = (config->MaxBlockSize < minBlockSize) ? minBlockSize : config->MaxBlockSize;

    struct BlockSequenceConfig blockConfig = {
        .BlockDevice = config->BlockDevice,
        .MaxBlockSize = maxBlockSize,
        .MaxBlocks = config->MaxBlocks,
        .DiscardPolicy = config->DiscardPolicy,
        .OnStoreFull = config->OnStoreFull,
        .StoreFullContext = config->StoreFullContext,
        .GetCapacityThreshold = config->GetCapacityThreshold,
        .OnThresholdCrossed = config->OnThresholdCrossed,
        .ThresholdContext = config->ThresholdContext,
    };
    return blockConfig;
}

static inline void BlockStore_InitialiseVtable(struct SolidSyslogBlockStore* blockStore)
{
    blockStore->Base.Write = BlockStore_Write;
    blockStore->Base.ReadNextUnsent = BlockStore_ReadNextUnsent;
    blockStore->Base.MarkSent = BlockStore_MarkSent;
    blockStore->Base.HasUnsent = BlockStore_HasUnsent;
    blockStore->Base.IsHalted = BlockStore_IsHalted;
    blockStore->Base.GetTotalBytes = BlockStore_GetTotalBytes;
    blockStore->Base.GetUsedBytes = BlockStore_GetUsedBytes;
    blockStore->Base.IsTransient = BlockStore_IsTransient;
}

static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* blockStore)
{
    struct SolidSyslogBlockDevice* device = BlockSequence_BlockDevice(&blockStore->BlockSequence);
    size_t readSequence = BlockSequence_ReadSequence(&blockStore->BlockSequence);
    /* Bound the scan by the read block's actual size, not WritePosition. On a
     * multi-block resume the read block is a closed earlier block whose size
     * is independent of the write block's fill level. */
    size_t readBlockSize = SolidSyslogBlockDevice_Size(device, readSequence);

    bool corrupt = false;
    size_t cursor =
        RecordStore_FindFirstUnsent(&blockStore->RecordStore, device, readSequence, readBlockSize, &corrupt);

    BlockSequence_SetReadCursor(&blockStore->BlockSequence, cursor);

    if (corrupt)
    {
        BlockSequence_MarkWriteBlockCorrupt(&blockStore->BlockSequence);
    }
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore* store)
{
    struct SolidSyslogBlockStore* blockStore = BlockStore_AsBlockStore(store);
    *blockStore = DEFAULT_INSTANCE;
}

/* ------------------------------------------------------------------
 * BlockStore_Write
 * ----------------------------------------------------------------*/

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* blockStore, const void* data, size_t size);

static bool BlockStore_Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    return BlockStore_StoreRecord(BlockStore_AsBlockStore(self), data, size);
}

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* blockStore, const void* data, size_t size)
{
    size_t recordSize = RecordStore_RecordSize(&blockStore->RecordStore, (uint16_t) size);
    bool readBlockChanged = false;
    bool written = false;

    if (BlockSequence_PrepareForWrite(&blockStore->BlockSequence, recordSize, &readBlockChanged))
    {
        if (readBlockChanged)
        {
            RecordStore_ForgetLastRead(&blockStore->RecordStore);
        }

        if (RecordStore_Append(
                &blockStore->RecordStore,
                BlockSequence_BlockDevice(&blockStore->BlockSequence),
                BlockSequence_WriteSequence(&blockStore->BlockSequence),
                data,
                size
            ))
        {
            BlockSequence_NoteRecordWritten(&blockStore->BlockSequence, recordSize);
            written = true;
        }
    }

    return written;
}

/* ------------------------------------------------------------------
 * BlockStore_HasUnsent / BlockStore_IsHalted / BlockStore_GetTotalBytes / BlockStore_GetUsedBytes
 * ----------------------------------------------------------------*/

static bool BlockStore_HasUnsent(struct SolidSyslogStore* self)
{
    return BlockSequence_HasUnsent(&BlockStore_AsBlockStore(self)->BlockSequence);
}

static bool BlockStore_IsHalted(struct SolidSyslogStore* self)
{
    return BlockSequence_IsHalted(&BlockStore_AsBlockStore(self)->BlockSequence);
}

static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* self)
{
    return BlockSequence_TotalBytes(&BlockStore_AsBlockStore(self)->BlockSequence);
}

static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* self)
{
    return BlockSequence_UsedBytes(&BlockStore_AsBlockStore(self)->BlockSequence);
}

/* BlockStore retains records — a BlockStore_Write rejection here is the discard
 * policy speaking (DISCARD_NEWEST or HALT), and the message must NOT
 * bypass older stored records via a Service direct-send fallback. */
static bool BlockStore_IsTransient(struct SolidSyslogStore* self)
{
    (void) self;
    return false;
}

/* ------------------------------------------------------------------
 * BlockStore_ReadNextUnsent
 * ----------------------------------------------------------------*/

static bool BlockStore_ReadCurrent(
    struct SolidSyslogBlockStore* blockStore,
    void* data,
    size_t maxSize,
    size_t* bytesRead
);

static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogBlockStore* blockStore = BlockStore_AsBlockStore(self);
    bool read = false;
    *bytesRead = 0;

    if (BlockSequence_HasUnsent(&blockStore->BlockSequence))
    {
        read = BlockStore_ReadCurrent(blockStore, data, maxSize, bytesRead);

        while (!read && BlockSequence_ReadIsBehindWrite(&blockStore->BlockSequence))
        {
            BlockSequence_AdvanceToNextReadBlock(&blockStore->BlockSequence);
            RecordStore_ForgetLastRead(&blockStore->RecordStore);
            read = BlockStore_ReadCurrent(blockStore, data, maxSize, bytesRead);
        }
    }

    return read;
}

static bool BlockStore_ReadCurrent(
    struct SolidSyslogBlockStore* blockStore,
    void* data,
    size_t maxSize,
    size_t* bytesRead
)
{
    return RecordStore_Read(
        &blockStore->RecordStore,
        BlockSequence_BlockDevice(&blockStore->BlockSequence),
        BlockSequence_ReadSequence(&blockStore->BlockSequence),
        BlockSequence_ReadCursor(&blockStore->BlockSequence),
        data,
        maxSize,
        bytesRead
    );
}

/* ------------------------------------------------------------------
 * BlockStore_MarkSent
 * ----------------------------------------------------------------*/

static void BlockStore_MarkSent(struct SolidSyslogStore* self)
{
    struct SolidSyslogBlockStore* blockStore = BlockStore_AsBlockStore(self);
    size_t nextCursor = 0;

    if (RecordStore_MarkLastReadAsSent(
            &blockStore->RecordStore,
            BlockSequence_BlockDevice(&blockStore->BlockSequence),
            &nextCursor
        ))
    {
        BlockSequence_SetReadCursor(&blockStore->BlockSequence, nextCursor);

        bool readBlockChanged = false;
        BlockSequence_DisposeReadBlockIfDrained(&blockStore->BlockSequence, &readBlockChanged);

        if (readBlockChanged)
        {
            RecordStore_ForgetLastRead(&blockStore->RecordStore);
        }
    }
}
