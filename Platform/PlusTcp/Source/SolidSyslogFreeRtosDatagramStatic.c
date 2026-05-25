#include "SolidSyslogFreeRtosDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosDatagramErrors.h"
#include "SolidSyslogFreeRtosDatagramPrivate.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t FreeRtosDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void FreeRtosDatagram_CleanupAtIndex(size_t index, void* context);

static bool FreeRtosDatagram_InUse[SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE];
static struct SolidSyslogFreeRtosDatagram FreeRtosDatagram_Pool[SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator FreeRtosDatagram_Allocator = {
    FreeRtosDatagram_InUse,
    SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogFreeRtosDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FreeRtosDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosDatagram_Allocator, index) == true)
    {
        FreeRtosDatagram_Initialise(&FreeRtosDatagram_Pool[index].Base);
        handle = &FreeRtosDatagram_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &FreeRtosDatagramErrorSource,
            (uint8_t) FREERTOSDATAGRAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogFreeRtosDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = FreeRtosDatagram_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosDatagram_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&FreeRtosDatagram_Allocator, index, FreeRtosDatagram_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &FreeRtosDatagramErrorSource,
            (uint8_t) FREERTOSDATAGRAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t FreeRtosDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &FreeRtosDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FreeRtosDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FreeRtosDatagram_Cleanup(&FreeRtosDatagram_Pool[index].Base);
}
