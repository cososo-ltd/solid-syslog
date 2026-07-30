#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBlockSequencePrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogTunables.h"

static inline size_t BlockSequence_IndexFromHandle(const struct SolidSyslogBlockSequence* blockSequence);
static inline void BlockSequence_CleanupAtIndex(size_t index, void* context);

static bool BlockSequence_InUse[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogBlockSequence BlockSequence_Pool[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogPoolAllocator BlockSequence_Allocator = {
    BlockSequence_InUse,
    SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE
};

struct SolidSyslogBlockSequence* SolidSyslogBlockSequence_Create(const struct SolidSyslogBlockSequenceConfig* config)
{
    struct SolidSyslogBlockSequence* result = NULL;
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&BlockSequence_Allocator);
    if (SolidSyslogPoolAllocator_IndexIsValid(&BlockSequence_Allocator, index))
    {
        SolidSyslogBlockSequence_Initialise(&BlockSequence_Pool[index], config);
        result = &BlockSequence_Pool[index];
    }
    return result;
}

void SolidSyslogBlockSequence_Destroy(struct SolidSyslogBlockSequence* blockSequence)
{
    if (blockSequence != NULL)
    {
        size_t index = BlockSequence_IndexFromHandle(blockSequence);
        if (SolidSyslogPoolAllocator_IndexIsValid(&BlockSequence_Allocator, index))
        {
            (void
            ) SolidSyslogPoolAllocator_FreeIfInUse(&BlockSequence_Allocator, index, BlockSequence_CleanupAtIndex, NULL);
        }
    }
}

static inline size_t BlockSequence_IndexFromHandle(const struct SolidSyslogBlockSequence* blockSequence)
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

static inline void BlockSequence_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogBlockSequence_Cleanup(&BlockSequence_Pool[index]);
}
