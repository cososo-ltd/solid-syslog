#include "SolidSyslogLwipRawTcpStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawTcpStreamErrors.h"
#include "SolidSyslogLwipRawTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline bool LwipRawTcpStream_IsValidConfig(const struct SolidSyslogLwipRawTcpStreamConfig* config);
static inline size_t LwipRawTcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void LwipRawTcpStream_CleanupAtIndex(size_t index, void* context);

static bool LwipRawTcpStream_InUse[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogLwipRawTcpStream LwipRawTcpStream_Pool[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipRawTcpStream_Allocator = {
    LwipRawTcpStream_InUse,
    SOLIDSYSLOG_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogLwipRawTcpStream_Create(const struct SolidSyslogLwipRawTcpStreamConfig* config)
{
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (LwipRawTcpStream_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipRawTcpStream_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&LwipRawTcpStream_Allocator, index) == true)
        {
            LwipRawTcpStream_Initialise(&LwipRawTcpStream_Pool[index].Base, config);
            handle = &LwipRawTcpStream_Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &LwipRawTcpStreamErrorSource,
                (uint8_t) LWIPRAWTCPSTREAM_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

void SolidSyslogLwipRawTcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = LwipRawTcpStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&LwipRawTcpStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&LwipRawTcpStream_Allocator, index, LwipRawTcpStream_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &LwipRawTcpStreamErrorSource,
            (uint8_t) LWIPRAWTCPSTREAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline bool LwipRawTcpStream_IsValidConfig(const struct SolidSyslogLwipRawTcpStreamConfig* config)
{
    return (config != NULL) && (config->Sleep != NULL);
}

static inline size_t LwipRawTcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipRawTcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipRawTcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    LwipRawTcpStream_Cleanup(&LwipRawTcpStream_Pool[index].Base);
}
