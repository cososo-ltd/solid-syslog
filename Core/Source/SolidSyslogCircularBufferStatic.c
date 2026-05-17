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

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    struct SolidSyslogBuffer* result = &Fallback;
    SolidSyslog_LockConfig();
    for (size_t i = 0; i < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE; i++)
    {
        if (!Pool[i].InUse)
        {
            Pool[i].InUse = true;
            CircularBuffer_Initialise(&Pool[i].Object, mutex, ring, ringBytes);
            result = &Pool[i].Object.Base;
            break;
        }
    }
    SolidSyslog_UnlockConfig();
    if (result == &Fallback)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_POOL_EXHAUSTED);
    }
    return result;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    if (base == &Fallback)
    {
        return;
    }
    bool released = false;
    SolidSyslog_LockConfig();
    for (size_t i = 0; i < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE; i++)
    {
        if (Pool[i].InUse && (base == &Pool[i].Object.Base))
        {
            CircularBuffer_Cleanup(base);
            Pool[i].InUse = false;
            released = true;
            break;
        }
    }
    SolidSyslog_UnlockConfig();
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY);
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
