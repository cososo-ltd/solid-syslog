#include "SolidSyslogFreeRtosTcpStream.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline size_t FreeRtosTcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void FreeRtosTcpStream_CleanupAtIndex(size_t index, void* context);

static bool FreeRtosTcpStream_InUse[SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogFreeRtosTcpStream FreeRtosTcpStream_Pool[SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator FreeRtosTcpStream_Allocator = {
    FreeRtosTcpStream_InUse,
    SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogFreeRtosTcpStream_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FreeRtosTcpStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosTcpStream_Allocator, index) == true)
    {
        FreeRtosTcpStream_Initialise(&FreeRtosTcpStream_Pool[index].Base);
        handle = &FreeRtosTcpStream_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogFreeRtosTcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = FreeRtosTcpStream_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosTcpStream_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &FreeRtosTcpStream_Allocator,
                        index,
                        FreeRtosTcpStream_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_UNKNOWN_DESTROY);
    }
}

static inline size_t FreeRtosTcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &FreeRtosTcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FreeRtosTcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FreeRtosTcpStream_Cleanup(&FreeRtosTcpStream_Pool[index].Base);
}
