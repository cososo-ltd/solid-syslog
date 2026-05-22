#include "SolidSyslogPosixAddress.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* PosixAddress_HandleFromIndex(size_t index);
static inline size_t PosixAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void PosixAddress_CleanupAtIndex(size_t index, void* context);

static bool PosixAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPosixAddress PosixAddress_Pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixAddress_Allocator = {PosixAddress_InUse, SOLIDSYSLOG_ADDRESS_POOL_SIZE};

/* TU-private fallback returned when the pool is exhausted. Sized as a real
 * SolidSyslogPosixAddress so a Resolver overwrite at the exhausted-fallback
 * call site is bounded — same sockaddr_in storage as any pooled slot. Not
 * a per-Sender slot: multi-overflow integrators share this storage and
 * race on it. Bumping SOLIDSYSLOG_ADDRESS_POOL_SIZE removes the race. */
static struct SolidSyslogPosixAddress PosixAddress_Fallback;

struct SolidSyslogAddress* SolidSyslogPosixAddress_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &PosixAddress_Fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixAddress_Allocator, index))
    {
        handle = PosixAddress_HandleFromIndex(index);
        PosixAddress_Initialise(handle);
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_POSIXADDRESS_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogPosixAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = PosixAddress_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PosixAddress_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PosixAddress_Allocator, index, PosixAddress_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_POSIXADDRESS_UNKNOWN_DESTROY);
    }
}

static inline struct SolidSyslogAddress* PosixAddress_HandleFromIndex(size_t index)
{
    return (struct SolidSyslogAddress*) &PosixAddress_Pool[index];
}

static inline size_t PosixAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == PosixAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixAddress_Cleanup(PosixAddress_HandleFromIndex(index));
}
