#include "SolidSyslogWinsockAddress.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockAddressPrivate.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* WinsockAddress_HandleFromIndex(size_t index);
static inline size_t WinsockAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void WinsockAddress_CleanupAtIndex(size_t index, void* context);

static bool WinsockAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogWinsockAddress WinsockAddress_Pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator WinsockAddress_Allocator = {WinsockAddress_InUse, SOLIDSYSLOG_ADDRESS_POOL_SIZE};

/* TU-private fallback returned when the pool is exhausted. Sized as a real
 * SolidSyslogWinsockAddress so a Resolver overwrite at the exhausted-fallback
 * call site is bounded — same sockaddr_in storage as any pooled slot. Not
 * a per-Sender slot: multi-overflow integrators share this storage and
 * race on it. Bumping SOLIDSYSLOG_ADDRESS_POOL_SIZE removes the race. */
static struct SolidSyslogWinsockAddress WinsockAddress_Fallback;

struct SolidSyslogAddress* SolidSyslogWinsockAddress_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WinsockAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &WinsockAddress_Fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&WinsockAddress_Allocator, index))
    {
        handle = WinsockAddress_HandleFromIndex(index);
        WinsockAddress_Initialise(handle);
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_WINSOCKADDRESS_POOL_EXHAUSTED);
    }
    return handle;
}

static inline struct SolidSyslogAddress* WinsockAddress_HandleFromIndex(size_t index)
{
    return (struct SolidSyslogAddress*) &WinsockAddress_Pool[index];
}

void SolidSyslogWinsockAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = WinsockAddress_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WinsockAddress_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WinsockAddress_Allocator, index, WinsockAddress_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_WINSOCKADDRESS_UNKNOWN_DESTROY);
    }
}

static inline size_t WinsockAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == WinsockAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WinsockAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WinsockAddress_Cleanup(WinsockAddress_HandleFromIndex(index));
}
