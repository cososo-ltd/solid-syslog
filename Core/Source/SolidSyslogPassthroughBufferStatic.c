#include "SolidSyslogPassthroughBuffer.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPassthroughBufferPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogSender;

static bool Fallback_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void Fallback_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);
static size_t PassthroughBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base);
static void PassthroughBuffer_CleanupAtIndex(size_t index, void* context);

static bool InUse[SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE];
static struct SolidSyslogPassthroughBuffer Pool[SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE];
static struct SolidSyslogBuffer Fallback = {Fallback_Write, Fallback_Read};
static struct SolidSyslogPoolAllocator Allocator = {InUse, SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE};

struct SolidSyslogBuffer* SolidSyslogPassthroughBuffer_Create(struct SolidSyslogSender* sender)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&Allocator);
    struct SolidSyslogBuffer* handle = &Fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index))
    {
        PassthroughBuffer_Initialise(&Pool[index].Base, sender);
        handle = &Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_PASSTHROUGHBUFFER_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogPassthroughBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    size_t index = PassthroughBuffer_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&Allocator, index, PassthroughBuffer_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_PASSTHROUGHBUFFER_UNKNOWN_DESTROY);
    }
}

static size_t PassthroughBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base)
{
    size_t result = SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE; poolIndex++)
    {
        if (base == &Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void PassthroughBuffer_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PassthroughBuffer_Cleanup(&Pool[index].Base);
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
