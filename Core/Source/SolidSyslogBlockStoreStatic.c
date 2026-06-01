#include "SolidSyslogBlockStore.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "BlockSequencePrivate.h"
#include "RecordStorePrivate.h"
#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogBlockStorePrivate.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStore;

static inline size_t BlockStore_IndexFromHandle(const struct SolidSyslogStore* base);
static inline void BlockStore_CleanupAtIndex(size_t index, void* context);
static struct SolidSyslogSecurityPolicy* BlockStore_ResolveSecurityPolicy(struct SolidSyslogSecurityPolicy* configured);
static struct BlockSequenceConfig BlockStore_BuildBlockSequenceConfig(
    const struct SolidSyslogBlockStoreConfig* config,
    const struct RecordStore* recordStore
);

static bool BlockStore_InUse[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogBlockStore BlockStore_Pool[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogPoolAllocator BlockStore_Allocator = {BlockStore_InUse, SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE};

struct SolidSyslogStore* SolidSyslogBlockStore_Create(const struct SolidSyslogBlockStoreConfig* config)
{
    struct SolidSyslogStore* result = SolidSyslogNullStore_Get();
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&BlockStore_Allocator);

    if (SolidSyslogPoolAllocator_IndexIsValid(&BlockStore_Allocator, index))
    {
        struct SolidSyslogSecurityPolicy* policy = BlockStore_ResolveSecurityPolicy(config->SecurityPolicy);
        struct RecordStore* recordStore = RecordStore_Create(policy);

        if (recordStore != NULL)
        {
            struct BlockSequenceConfig blockConfig = BlockStore_BuildBlockSequenceConfig(config, recordStore);
            struct BlockSequence* blockSequence = BlockSequence_Create(&blockConfig);

            if (blockSequence != NULL)
            {
                BlockStore_Initialise(&BlockStore_Pool[index].Base, recordStore, blockSequence, config);
                result = &BlockStore_Pool[index].Base;
            }
            else
            {
                RecordStore_Destroy(recordStore);
                (void) SolidSyslogPoolAllocator_FreeIfInUse(&BlockStore_Allocator, index, NULL, NULL);
            }
        }
        else
        {
            (void) SolidSyslogPoolAllocator_FreeIfInUse(&BlockStore_Allocator, index, NULL, NULL);
        }
    }

    if (result == SolidSyslogNullStore_Get())
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &BlockStoreErrorSource,
            (uint8_t) BLOCKSTORE_ERROR_POOL_EXHAUSTED
        );
    }

    return result;
}

static struct SolidSyslogSecurityPolicy* BlockStore_ResolveSecurityPolicy(struct SolidSyslogSecurityPolicy* configured)
{
    struct SolidSyslogSecurityPolicy* resolved = configured;

    if ((resolved == NULL) || (resolved->TrailerSize > SOLIDSYSLOG_MAX_INTEGRITY_SIZE))
    {
        resolved = SolidSyslogNullSecurityPolicy_Get();
    }

    return resolved;
}

static struct BlockSequenceConfig BlockStore_BuildBlockSequenceConfig(
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

void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore* base)
{
    size_t index = BlockStore_IndexFromHandle(base);
    bool released = false;

    if (SolidSyslogPoolAllocator_IndexIsValid(&BlockStore_Allocator, index))
    {
        /* Pull the inner pool pointers out of the slot before FreeIfInUse runs
         * the BlockStore_Cleanup callback, because Cleanup overwrites the
         * abstract base with the NullStore vtable but the derived RecordStore /
         * BlockSequence pointers stay in the slot. After the outer FreeIfInUse
         * releases the ConfigLock we destroy the inner slots — keeps each pool's
         * lock acquisition sequential rather than nested. */
        struct RecordStore* recordStore = BlockStore_Pool[index].RecordStore;
        struct BlockSequence* blockSequence = BlockStore_Pool[index].BlockSequence;

        released = SolidSyslogPoolAllocator_FreeIfInUse(&BlockStore_Allocator, index, BlockStore_CleanupAtIndex, NULL);

        if (released)
        {
            BlockSequence_Destroy(blockSequence);
            RecordStore_Destroy(recordStore);
        }
    }

    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &BlockStoreErrorSource,
            (uint8_t) BLOCKSTORE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t BlockStore_IndexFromHandle(const struct SolidSyslogStore* base)
{
    size_t result = SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE; poolIndex++)
    {
        if (base == &BlockStore_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void BlockStore_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    BlockStore_Cleanup(&BlockStore_Pool[index].Base);
}
