#include "SolidSyslogFreeRtosResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosResolverErrors.h"
#include "SolidSyslogFreeRtosResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t FreeRtosResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void FreeRtosResolver_CleanupAtIndex(size_t index, void* context);

static bool FreeRtosResolver_InUse[SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE];
static struct SolidSyslogFreeRtosResolver FreeRtosResolver_Pool[SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator FreeRtosResolver_Allocator = {
    FreeRtosResolver_InUse,
    SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogFreeRtosResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FreeRtosResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosResolver_Allocator, index) == true)
    {
        FreeRtosResolver_Initialise(&FreeRtosResolver_Pool[index].Base);
        handle = &FreeRtosResolver_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &FreeRtosResolverErrorSource,
            (uint8_t) FREERTOSRESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogFreeRtosResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = FreeRtosResolver_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosResolver_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&FreeRtosResolver_Allocator, index, FreeRtosResolver_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &FreeRtosResolverErrorSource,
            (uint8_t) FREERTOSRESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t FreeRtosResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &FreeRtosResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FreeRtosResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FreeRtosResolver_Cleanup(&FreeRtosResolver_Pool[index].Base);
}
