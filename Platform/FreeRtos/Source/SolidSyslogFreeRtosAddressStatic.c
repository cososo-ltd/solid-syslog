#include "SolidSyslogFreeRtosAddress.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosAddressPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* FreeRtosAddress_HandleFromIndex(size_t index);
static inline size_t FreeRtosAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void FreeRtosAddress_CleanupAtIndex(size_t index, void* context);

static bool FreeRtosAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogFreeRtosAddress FreeRtosAddress_Pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator FreeRtosAddress_Allocator = {
    FreeRtosAddress_InUse,
    SOLIDSYSLOG_ADDRESS_POOL_SIZE
};

/* TU-private fallback returned when the pool is exhausted. Sized as a real
 * SolidSyslogFreeRtosAddress so a Resolver overwrite at the exhausted-fallback
 * call site is bounded — same freertos_sockaddr storage as any pooled slot.
 * Not a per-Sender slot: multi-overflow integrators share this storage and
 * race on it. Bumping SOLIDSYSLOG_ADDRESS_POOL_SIZE removes the race. */
static struct SolidSyslogFreeRtosAddress FreeRtosAddress_Fallback;

struct SolidSyslogAddress* SolidSyslogFreeRtosAddress_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FreeRtosAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &FreeRtosAddress_Fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosAddress_Allocator, index))
    {
        handle = FreeRtosAddress_HandleFromIndex(index);
        FreeRtosAddress_Initialise(handle);
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_POOL_EXHAUSTED);
    }
    return handle;
}

static inline struct SolidSyslogAddress* FreeRtosAddress_HandleFromIndex(size_t index)
{
    return (struct SolidSyslogAddress*) &FreeRtosAddress_Pool[index];
}

void SolidSyslogFreeRtosAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = FreeRtosAddress_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosAddress_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&FreeRtosAddress_Allocator, index, FreeRtosAddress_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_UNKNOWN_DESTROY);
    }
}

static inline size_t FreeRtosAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == FreeRtosAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FreeRtosAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FreeRtosAddress_Cleanup(FreeRtosAddress_HandleFromIndex(index));
}
