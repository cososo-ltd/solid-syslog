#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogRecordStorePrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogSecurityPolicy;

static inline size_t RecordStore_IndexFromHandle(const struct SolidSyslogRecordStore* recordStore);
static inline void RecordStore_CleanupAtIndex(size_t index, void* context);

static bool RecordStore_InUse[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogRecordStore RecordStore_Pool[SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE];
static struct SolidSyslogPoolAllocator RecordStore_Allocator = {RecordStore_InUse, SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE};

struct SolidSyslogRecordStore* SolidSyslogRecordStore_Create(struct SolidSyslogSecurityPolicy* securityPolicy)
{
    struct SolidSyslogRecordStore* result = NULL;
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&RecordStore_Allocator);
    if (SolidSyslogPoolAllocator_IndexIsValid(&RecordStore_Allocator, index))
    {
        SolidSyslogRecordStore_Initialise(&RecordStore_Pool[index], securityPolicy);
        result = &RecordStore_Pool[index];
    }
    return result;
}

void SolidSyslogRecordStore_Destroy(struct SolidSyslogRecordStore* recordStore)
{
    if (recordStore != NULL)
    {
        size_t index = RecordStore_IndexFromHandle(recordStore);
        if (SolidSyslogPoolAllocator_IndexIsValid(&RecordStore_Allocator, index))
        {
            (void
            ) SolidSyslogPoolAllocator_FreeIfInUse(&RecordStore_Allocator, index, RecordStore_CleanupAtIndex, NULL);
        }
    }
}

static inline size_t RecordStore_IndexFromHandle(const struct SolidSyslogRecordStore* recordStore)
{
    size_t result = SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE; poolIndex++)
    {
        if (recordStore == &RecordStore_Pool[poolIndex])
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void RecordStore_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogRecordStore_Cleanup(&RecordStore_Pool[index]);
}
