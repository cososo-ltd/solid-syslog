#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBufferPrivate.h"
#include "SolidSyslogConfigLock.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;

struct Slot
{
    struct SolidSyslogCircularBuffer Object;
    bool InUse;
};

static bool Fallback_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void Fallback_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);
static struct SolidSyslogBuffer* CircularBuffer_AcquireFirstFree(void);
static struct SolidSyslogBuffer* CircularBuffer_AcquireIfFree(size_t poolIndex);
static inline bool CircularBuffer_PoolItemIsFree(size_t poolIndex);
static inline bool CircularBuffer_PoolItemIsInUse(size_t poolIndex);
static inline struct SolidSyslogBuffer* CircularBuffer_Acquire(size_t poolIndex);
static inline void CircularBuffer_MarkInUse(size_t poolIndex);
static inline struct SolidSyslogBuffer* CircularBuffer_HandleFromIndex(size_t poolIndex);
static inline bool CircularBuffer_HandleIsValid(const struct SolidSyslogBuffer* handle);
static size_t CircularBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base);
static inline bool CircularBuffer_PoolIndexIsValid(size_t poolIndex);
static bool CircularBuffer_FreeIfInUse(size_t poolIndex);
static inline void CircularBuffer_MarkFree(size_t poolIndex);

static struct Slot Pool[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE];
static struct SolidSyslogBuffer Fallback = {Fallback_Write, Fallback_Read};

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    struct SolidSyslogBuffer* handle = CircularBuffer_AcquireFirstFree();
    if (CircularBuffer_HandleIsValid(handle))
    {
        CircularBuffer_Initialise(handle, mutex, ring, ringBytes);
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_POOL_EXHAUSTED);
    }
    return handle;
}

static struct SolidSyslogBuffer* CircularBuffer_AcquireFirstFree(void)
{
    struct SolidSyslogBuffer* handle = &Fallback;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE; poolIndex++)
    {
        handle = CircularBuffer_AcquireIfFree(poolIndex);
        if (CircularBuffer_HandleIsValid(handle))
        {
            break;
        }
    }
    return handle;
}

static struct SolidSyslogBuffer* CircularBuffer_AcquireIfFree(size_t poolIndex)
{
    struct SolidSyslogBuffer* handle = &Fallback;
    SolidSyslog_LockConfig();
    if (CircularBuffer_PoolItemIsFree(poolIndex))
    {
        handle = CircularBuffer_Acquire(poolIndex);
    }
    SolidSyslog_UnlockConfig();
    return handle;
}

static inline bool CircularBuffer_PoolItemIsFree(size_t poolIndex)
{
    return !CircularBuffer_PoolItemIsInUse(poolIndex);
}

static inline bool CircularBuffer_PoolItemIsInUse(size_t poolIndex)
{
    return Pool[poolIndex].InUse;
}

static inline struct SolidSyslogBuffer* CircularBuffer_Acquire(size_t poolIndex)
{
    CircularBuffer_MarkInUse(poolIndex);
    return CircularBuffer_HandleFromIndex(poolIndex);
}

static inline void CircularBuffer_MarkInUse(size_t poolIndex)
{
    Pool[poolIndex].InUse = true;
}

static inline struct SolidSyslogBuffer* CircularBuffer_HandleFromIndex(size_t poolIndex)
{
    return &Pool[poolIndex].Object.Base;
}

static inline bool CircularBuffer_HandleIsValid(const struct SolidSyslogBuffer* handle)
{
    return handle != &Fallback;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    size_t poolIndex = CircularBuffer_IndexFromHandle(base);
    bool released = false;
    if (CircularBuffer_PoolIndexIsValid(poolIndex))
    {
        released = CircularBuffer_FreeIfInUse(poolIndex);
    }
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
        if (base == CircularBuffer_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline bool CircularBuffer_PoolIndexIsValid(size_t poolIndex)
{
    return poolIndex < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE;
}

static bool CircularBuffer_FreeIfInUse(size_t poolIndex)
{
    bool released = false;
    SolidSyslog_LockConfig();
    if (CircularBuffer_PoolItemIsInUse(poolIndex))
    {
        CircularBuffer_Cleanup(CircularBuffer_HandleFromIndex(poolIndex));
        CircularBuffer_MarkFree(poolIndex);
        released = true;
    }
    SolidSyslog_UnlockConfig();
    return released;
}

static inline void CircularBuffer_MarkFree(size_t poolIndex)
{
    Pool[poolIndex].InUse = false;
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
