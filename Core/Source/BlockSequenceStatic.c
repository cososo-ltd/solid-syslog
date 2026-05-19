#include <stdbool.h>
#include <stddef.h>

#include "BlockSequencePrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogTunables.h"

static size_t BlockSequence_IndexFromHandle(const struct BlockSequence* blockSequence);
static void BlockSequence_CleanupAtIndex(size_t index, void* context);

static bool BlockSequence_InUse[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct BlockSequence BlockSequence_Pool[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogPoolAllocator BlockSequence_Allocator = {
    BlockSequence_InUse,
    SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE
};

struct BlockSequence* BlockSequence_Create(const struct BlockSequenceConfig* config)
{
    struct BlockSequence* result = NULL;
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&BlockSequence_Allocator);
    if (SolidSyslogPoolAllocator_IndexIsValid(&BlockSequence_Allocator, index))
    {
        BlockSequence_Initialise(&BlockSequence_Pool[index], config);
        result = &BlockSequence_Pool[index];
    }
    return result;
}

void BlockSequence_Destroy(struct BlockSequence* blockSequence)
{
    if (blockSequence != NULL)
    {
        size_t index = BlockSequence_IndexFromHandle(blockSequence);
        if (SolidSyslogPoolAllocator_IndexIsValid(&BlockSequence_Allocator, index))
        {
            SolidSyslogPoolAllocator_FreeIfInUse(&BlockSequence_Allocator, index, BlockSequence_CleanupAtIndex, NULL);
        }
    }
}

static size_t BlockSequence_IndexFromHandle(const struct BlockSequence* blockSequence)
{
    size_t result = SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE; poolIndex++)
    {
        if (blockSequence == &BlockSequence_Pool[poolIndex])
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void BlockSequence_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    BlockSequence_Cleanup(&BlockSequence_Pool[index]);
}
