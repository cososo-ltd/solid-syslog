#include "SolidSyslogMbedTlsStream.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogMbedTlsStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline size_t MbedTlsStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void MbedTlsStream_CleanupAtIndex(size_t index, void* context);

static bool MbedTlsStream_InUse[SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE];
static struct SolidSyslogMbedTlsStream MbedTlsStream_Pool[SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator MbedTlsStream_Allocator = {
    MbedTlsStream_InUse,
    SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogMbedTlsStream_Create(const struct SolidSyslogMbedTlsStreamConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&MbedTlsStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&MbedTlsStream_Allocator, index))
    {
        MbedTlsStream_Initialise(&MbedTlsStream_Pool[index].Base, config);
        handle = &MbedTlsStream_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogMbedTlsStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = MbedTlsStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&MbedTlsStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&MbedTlsStream_Allocator, index, MbedTlsStream_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_UNKNOWN_DESTROY);
    }
}

static inline size_t MbedTlsStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &MbedTlsStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void MbedTlsStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    MbedTlsStream_Cleanup(&MbedTlsStream_Pool[index].Base);
}
