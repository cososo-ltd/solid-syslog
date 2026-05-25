#include "SolidSyslogPlusTcpTcpStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpTcpStreamErrors.h"
#include "SolidSyslogPlusTcpTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline size_t PlusTcpTcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void PlusTcpTcpStream_CleanupAtIndex(size_t index, void* context);

static bool PlusTcpTcpStream_InUse[SOLIDSYSLOG_PLUS_TCP_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPlusTcpTcpStream PlusTcpTcpStream_Pool[SOLIDSYSLOG_PLUS_TCP_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator PlusTcpTcpStream_Allocator = {
    PlusTcpTcpStream_InUse,
    SOLIDSYSLOG_PLUS_TCP_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogPlusTcpTcpStream_Create(const struct SolidSyslogPlusTcpTcpStreamConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PlusTcpTcpStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpTcpStream_Allocator, index) == true)
    {
        PlusTcpTcpStream_Initialise(&PlusTcpTcpStream_Pool[index].Base, config);
        handle = &PlusTcpTcpStream_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PlusTcpTcpStreamErrorSource,
            (uint8_t) PLUSTCPTCPSTREAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPlusTcpTcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = PlusTcpTcpStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpTcpStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PlusTcpTcpStream_Allocator, index, PlusTcpTcpStream_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &PlusTcpTcpStreamErrorSource,
            (uint8_t) PLUSTCPTCPSTREAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PlusTcpTcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_PLUS_TCP_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_PLUS_TCP_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &PlusTcpTcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PlusTcpTcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PlusTcpTcpStream_Cleanup(&PlusTcpTcpStream_Pool[index].Base);
}
