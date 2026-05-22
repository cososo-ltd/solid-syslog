#include "SolidSyslogWinsockDatagram.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockDatagramPrivate.h"

struct SolidSyslogDatagram;

static inline size_t WinsockDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void WinsockDatagram_CleanupAtIndex(size_t index, void* context);

static bool WinsockDatagram_InUse[SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE];
static struct SolidSyslogWinsockDatagram WinsockDatagram_Pool[SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator WinsockDatagram_Allocator = {
    WinsockDatagram_InUse,
    SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogWinsockDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WinsockDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&WinsockDatagram_Allocator, index) == true)
    {
        WinsockDatagram_Initialise(&WinsockDatagram_Pool[index].Base);
        handle = &WinsockDatagram_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_WINSOCKDATAGRAM_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogWinsockDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = WinsockDatagram_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WinsockDatagram_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WinsockDatagram_Allocator, index, WinsockDatagram_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_WINSOCKDATAGRAM_UNKNOWN_DESTROY);
    }
}

static inline size_t WinsockDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &WinsockDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WinsockDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WinsockDatagram_Cleanup(&WinsockDatagram_Pool[index].Base);
}
