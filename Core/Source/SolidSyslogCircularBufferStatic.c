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

static struct Slot Pool[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE];
static struct SolidSyslogBuffer Fallback = {Fallback_Write, Fallback_Read};

static struct SolidSyslogBuffer* CircularBuffer_AcquireIfFree(size_t i)
{
    struct SolidSyslogBuffer* handle = &Fallback;
    SolidSyslog_LockConfig();
    if (!Pool[i].InUse)
    {
        Pool[i].InUse = true;
        handle = &Pool[i].Object.Base;
    }
    SolidSyslog_UnlockConfig();
    return handle;
}

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    struct SolidSyslogBuffer* handle = &Fallback;
    for (size_t i = 0; i < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE; i++)
    {
        handle = CircularBuffer_AcquireIfFree(i);
        if (handle != &Fallback)
        {
            break;
        }
    }
    if (handle != &Fallback)
    {
        CircularBuffer_Initialise(handle, mutex, ring, ringBytes);
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    if (base != &Fallback)
    {
        bool released = false;
        for (size_t i = 0; (i < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE) && !released; i++)
        {
            SolidSyslog_LockConfig();
            if (Pool[i].InUse && (base == &Pool[i].Object.Base))
            {
                CircularBuffer_Cleanup(base);
                Pool[i].InUse = false;
                released = true;
            }
            SolidSyslog_UnlockConfig();
        }
        if (!released)
        {
            SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY);
        }
    }
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
