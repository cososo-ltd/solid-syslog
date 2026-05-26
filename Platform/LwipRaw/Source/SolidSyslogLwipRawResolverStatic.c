#include "SolidSyslogLwipRawResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawResolverErrors.h"
#include "SolidSyslogLwipRawResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t LwipRawResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void LwipRawResolver_CleanupAtIndex(size_t index, void* context);

static bool LwipRawResolver_InUse[SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE];
static struct SolidSyslogLwipRawResolver LwipRawResolver_Pool[SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipRawResolver_Allocator = {
    LwipRawResolver_InUse,
    SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogLwipRawResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipRawResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipRawResolver_Allocator, index) == true)
    {
        LwipRawResolver_Initialise(&LwipRawResolver_Pool[index].Base);
        handle = &LwipRawResolver_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &LwipRawResolverErrorSource,
            (uint8_t) LWIPRAWRESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLwipRawResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = LwipRawResolver_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&LwipRawResolver_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&LwipRawResolver_Allocator, index, LwipRawResolver_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &LwipRawResolverErrorSource,
            (uint8_t) LWIPRAWRESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipRawResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipRawResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipRawResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    LwipRawResolver_Cleanup(&LwipRawResolver_Pool[index].Base);
}
