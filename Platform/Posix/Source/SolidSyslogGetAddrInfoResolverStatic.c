#include "SolidSyslogGetAddrInfoResolver.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogGetAddrInfoResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t GetAddrInfoResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void GetAddrInfoResolver_CleanupAtIndex(size_t index, void* context);

static bool GetAddrInfoResolver_InUse[SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE];
static struct SolidSyslogGetAddrInfoResolver GetAddrInfoResolver_Pool[SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator GetAddrInfoResolver_Allocator = {
    GetAddrInfoResolver_InUse,
    SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogGetAddrInfoResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&GetAddrInfoResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&GetAddrInfoResolver_Allocator, index) == true)
    {
        GetAddrInfoResolver_Initialise(&GetAddrInfoResolver_Pool[index].Base);
        handle = &GetAddrInfoResolver_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_GETADDRINFORESOLVER_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogGetAddrInfoResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = GetAddrInfoResolver_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&GetAddrInfoResolver_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &GetAddrInfoResolver_Allocator,
                        index,
                        GetAddrInfoResolver_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_GETADDRINFORESOLVER_UNKNOWN_DESTROY);
    }
}

static inline size_t GetAddrInfoResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &GetAddrInfoResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void GetAddrInfoResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    GetAddrInfoResolver_Cleanup(&GetAddrInfoResolver_Pool[index].Base);
}
