#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBufferPrivate.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;

static bool Fallback_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void Fallback_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);
static size_t CircularBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base);
static void CircularBuffer_CleanupAtIndex(size_t index, void* context);

static bool CircularBuffer_InUse[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE];
static struct SolidSyslogCircularBuffer CircularBuffer_Pool[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE];
struct SolidSyslogBuffer CircularBuffer_Fallback = {Fallback_Write, Fallback_Read};
static struct SolidSyslogPoolAllocator CircularBuffer_Allocator = {
    CircularBuffer_InUse,
    SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE
};

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&CircularBuffer_Allocator);
    struct SolidSyslogBuffer* handle = &CircularBuffer_Fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&CircularBuffer_Allocator, index))
    {
        CircularBuffer_Initialise(&CircularBuffer_Pool[index].Base, mutex, ring, ringBytes);
        handle = &CircularBuffer_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    size_t index = CircularBuffer_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&CircularBuffer_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&CircularBuffer_Allocator, index, CircularBuffer_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY);
    }
}

static size_t CircularBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base)
{
    size_t result = SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE; poolIndex++)
    {
        if (base == &CircularBuffer_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void CircularBuffer_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    CircularBuffer_Cleanup(&CircularBuffer_Pool[index].Base);
}

static bool Fallback_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) base;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void Fallback_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    (void) base;
    (void) data;
    (void) size;
}
