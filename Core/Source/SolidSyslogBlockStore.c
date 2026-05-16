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
static bool BlockStore_Write(struct SolidSyslogStore* base, const void* data, size_t size);
static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead);
static void BlockStore_MarkSent(struct SolidSyslogStore* base);
static bool BlockStore_HasUnsent(struct SolidSyslogStore* base);
static bool BlockStore_IsHalted(struct SolidSyslogStore* base);
static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* base);
static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* base);
static bool BlockStore_IsTransient(struct SolidSyslogStore* base);

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

static inline struct SolidSyslogBlockStore* BlockStore_SelfFromStorage(SolidSyslogBlockStoreStorage* storage);
static inline struct SolidSyslogBlockStore* BlockStore_SelfFromBase(struct SolidSyslogStore* base);
static inline struct SolidSyslogSecurityPolicy* BlockStore_ResolveSecurityPolicy(
    struct SolidSyslogSecurityPolicy* configured
);
static inline struct BlockSequenceConfig BlockStore_BuildBlockSequenceConfig(
    const struct SolidSyslogBlockStoreConfig* config,
    const struct RecordStore* recordStore
);
static inline void BlockStore_InitialiseVtable(struct SolidSyslogBlockStore* self);
static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* self);

/* ------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------*/

struct SolidSyslogStore* SolidSyslogBlockStore_Create(
    SolidSyslogBlockStoreStorage* storage,
    const struct SolidSyslogBlockStoreConfig* config
)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;

    RecordStore_Init(&self->RecordStore, BlockStore_ResolveSecurityPolicy(config->SecurityPolicy));

    struct BlockSequenceConfig blockConfig = BlockStore_BuildBlockSequenceConfig(config, &self->RecordStore);
    BlockSequence_Init(&self->BlockSequence, &blockConfig);

    BlockStore_InitialiseVtable(self);

    if (BlockSequence_Open(&self->BlockSequence))
    {
        BlockStore_ResumeFromExistingBlock(self);
    }

    return &self->Base;
}

static inline struct SolidSyslogBlockStore* BlockStore_SelfFromStorage(SolidSyslogBlockStoreStorage* storage)
{
    return (struct SolidSyslogBlockStore*) storage;
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

static inline void BlockStore_InitialiseVtable(struct SolidSyslogBlockStore* self)
{
    self->Base.Write = BlockStore_Write;
    self->Base.ReadNextUnsent = BlockStore_ReadNextUnsent;
    self->Base.MarkSent = BlockStore_MarkSent;
    self->Base.HasUnsent = BlockStore_HasUnsent;
    self->Base.IsHalted = BlockStore_IsHalted;
    self->Base.GetTotalBytes = BlockStore_GetTotalBytes;
    self->Base.GetUsedBytes = BlockStore_GetUsedBytes;
    self->Base.IsTransient = BlockStore_IsTransient;
}

static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* self)
{
    struct SolidSyslogBlockDevice* device = BlockSequence_BlockDevice(&self->BlockSequence);
    size_t readSequence = BlockSequence_ReadSequence(&self->BlockSequence);
    /* Bound the scan by the read block's actual size, not WritePosition. On a
     * multi-block resume the read block is a closed earlier block whose size
     * is independent of the write block's fill level. */
    size_t readBlockSize = SolidSyslogBlockDevice_Size(device, readSequence);

    bool corrupt = false;
    size_t cursor = RecordStore_FindFirstUnsent(&self->RecordStore, device, readSequence, readBlockSize, &corrupt);

    BlockSequence_SetReadCursor(&self->BlockSequence, cursor);

    if (corrupt)
    {
        BlockSequence_MarkWriteBlockCorrupt(&self->BlockSequence);
    }
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore* base)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    *self = DEFAULT_INSTANCE;
}

static inline struct SolidSyslogBlockStore* BlockStore_SelfFromBase(struct SolidSyslogStore* base)
{
    return (struct SolidSyslogBlockStore*) base;
}

/* ------------------------------------------------------------------
 * BlockStore_Write
 * ----------------------------------------------------------------*/

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size);

static bool BlockStore_Write(struct SolidSyslogStore* base, const void* data, size_t size)
{
    return BlockStore_StoreRecord(BlockStore_SelfFromBase(base), data, size);
}

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size)
{
    size_t recordSize = RecordStore_RecordSize(&self->RecordStore, (uint16_t) size);
    bool readBlockChanged = false;
    bool written = false;

    if (BlockSequence_PrepareForWrite(&self->BlockSequence, recordSize, &readBlockChanged))
    {
        if (readBlockChanged)
        {
            RecordStore_ForgetLastRead(&self->RecordStore);
        }

        if (RecordStore_Append(
                &self->RecordStore,
                BlockSequence_BlockDevice(&self->BlockSequence),
                BlockSequence_WriteSequence(&self->BlockSequence),
                data,
                size
            ))
        {
            BlockSequence_NoteRecordWritten(&self->BlockSequence, recordSize);
            written = true;
        }
    }

    return written;
}

/* ------------------------------------------------------------------
 * BlockStore_HasUnsent / BlockStore_IsHalted / BlockStore_GetTotalBytes / BlockStore_GetUsedBytes
 * ----------------------------------------------------------------*/

static bool BlockStore_HasUnsent(struct SolidSyslogStore* base)
{
    return BlockSequence_HasUnsent(&BlockStore_SelfFromBase(base)->BlockSequence);
}

static bool BlockStore_IsHalted(struct SolidSyslogStore* base)
{
    return BlockSequence_IsHalted(&BlockStore_SelfFromBase(base)->BlockSequence);
}

static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* base)
{
    return BlockSequence_TotalBytes(&BlockStore_SelfFromBase(base)->BlockSequence);
}

static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* base)
{
    return BlockSequence_UsedBytes(&BlockStore_SelfFromBase(base)->BlockSequence);
}

/* BlockStore retains records — a BlockStore_Write rejection here is the discard
 * policy speaking (DISCARD_NEWEST or HALT), and the message must NOT
 * bypass older stored records via a Service direct-send fallback. */
static bool BlockStore_IsTransient(struct SolidSyslogStore* base)
{
    (void) base;
    return false;
}

/* ------------------------------------------------------------------
 * BlockStore_ReadNextUnsent
 * ----------------------------------------------------------------*/

static bool BlockStore_ReadCurrent(struct SolidSyslogBlockStore* self, void* data, size_t maxSize, size_t* bytesRead);

static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    bool read = false;
    *bytesRead = 0;

    if (BlockSequence_HasUnsent(&self->BlockSequence))
    {
        read = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);

        while (!read && BlockSequence_ReadIsBehindWrite(&self->BlockSequence))
        {
            BlockSequence_AdvanceToNextReadBlock(&self->BlockSequence);
            RecordStore_ForgetLastRead(&self->RecordStore);
            read = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);
        }
    }

    return read;
}

static bool BlockStore_ReadCurrent(struct SolidSyslogBlockStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    return RecordStore_Read(
        &self->RecordStore,
        BlockSequence_BlockDevice(&self->BlockSequence),
        BlockSequence_ReadSequence(&self->BlockSequence),
        BlockSequence_ReadCursor(&self->BlockSequence),
        data,
        maxSize,
        bytesRead
    );
}

/* ------------------------------------------------------------------
 * BlockStore_MarkSent
 * ----------------------------------------------------------------*/

static void BlockStore_MarkSent(struct SolidSyslogStore* base)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    size_t nextCursor = 0;

    if (RecordStore_MarkLastReadAsSent(
            &self->RecordStore,
            BlockSequence_BlockDevice(&self->BlockSequence),
            &nextCursor
        ))
    {
        BlockSequence_SetReadCursor(&self->BlockSequence, nextCursor);

        bool readBlockChanged = false;
        BlockSequence_DisposeReadBlockIfDrained(&self->BlockSequence, &readBlockChanged);

        if (readBlockChanged)
        {
            RecordStore_ForgetLastRead(&self->RecordStore);
        }
    }
}
