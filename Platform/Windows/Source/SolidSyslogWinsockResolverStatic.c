#include "SolidSyslogWinsockResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockResolverErrors.h"
#include "SolidSyslogWinsockResolverPrivate.h"

struct SolidSyslogResolver;

static inline size_t WinsockResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void WinsockResolver_CleanupAtIndex(size_t index, void* context);

static bool WinsockResolver_InUse[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogWinsockResolver WinsockResolver_Pool[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator WinsockResolver_Allocator = {
    WinsockResolver_InUse,
    SOLIDSYSLOG_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogWinsockResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WinsockResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&WinsockResolver_Allocator, index) == true)
    {
        WinsockResolver_Initialise(&WinsockResolver_Pool[index].Base);
        handle = &WinsockResolver_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &WinsockResolverErrorSource,
            (uint8_t) WINSOCKRESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogWinsockResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = WinsockResolver_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WinsockResolver_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WinsockResolver_Allocator, index, WinsockResolver_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &WinsockResolverErrorSource,
            (uint8_t) WINSOCKRESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WinsockResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &WinsockResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WinsockResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WinsockResolver_Cleanup(&WinsockResolver_Pool[index].Base);
}
