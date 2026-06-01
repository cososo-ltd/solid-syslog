#include "SolidSyslogWinsockTcpStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockTcpStreamErrors.h"
#include "SolidSyslogWinsockTcpStreamPrivate.h"

struct SolidSyslogStream;

static inline size_t WinsockTcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void WinsockTcpStream_CleanupAtIndex(size_t index, void* context);

static bool WinsockTcpStream_InUse[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogWinsockTcpStream WinsockTcpStream_Pool[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator WinsockTcpStream_Allocator = {
    WinsockTcpStream_InUse,
    SOLIDSYSLOG_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogWinsockTcpStream_Create(const struct SolidSyslogWinsockTcpStreamConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WinsockTcpStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&WinsockTcpStream_Allocator, index) == true)
    {
        WinsockTcpStream_Initialise(&WinsockTcpStream_Pool[index].Base, config);
        handle = &WinsockTcpStream_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &WinsockTcpStreamErrorSource,
            (uint8_t) WINSOCKTCPSTREAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogWinsockTcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = WinsockTcpStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WinsockTcpStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WinsockTcpStream_Allocator, index, WinsockTcpStream_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &WinsockTcpStreamErrorSource,
            (uint8_t) WINSOCKTCPSTREAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WinsockTcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &WinsockTcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WinsockTcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WinsockTcpStream_Cleanup(&WinsockTcpStream_Pool[index].Base);
}
