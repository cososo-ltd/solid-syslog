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

static inline struct SolidSyslogBuffer* CircularBuffer_TryClaim(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
);
static inline bool CircularBuffer_Release(struct SolidSyslogBuffer* base);
static inline bool CircularBuffer_SlotOwnsBase(const struct Slot* slot, const struct SolidSyslogBuffer* base);

static struct Slot Pool[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE];
static struct SolidSyslogBuffer Fallback = {Fallback_Write, Fallback_Read};

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    struct SolidSyslogBuffer* claimed = CircularBuffer_TryClaim(mutex, ring, ringBytes);
    if (claimed == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_POOL_EXHAUSTED);
        claimed = &Fallback;
    }
    return claimed;
}

static inline struct SolidSyslogBuffer* CircularBuffer_TryClaim(
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    size_t claimedIndex = SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE;
    for (size_t i = 0;
         (i < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE) && (claimedIndex == SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE);
         i++)
    {
        SolidSyslog_LockConfig();
        if (!Pool[i].InUse)
        {
            Pool[i].InUse = true;
            claimedIndex = i;
        }
        SolidSyslog_UnlockConfig();
    }
    struct SolidSyslogBuffer* claimed = NULL;
    if (claimedIndex < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE)
    {
        CircularBuffer_Initialise(&Pool[claimedIndex].Object, mutex, ring, ringBytes);
        claimed = &Pool[claimedIndex].Object.Base;
    }
    return claimed;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    if (base != &Fallback)
    {
        bool released = CircularBuffer_Release(base);
        if (!released)
        {
            SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY);
        }
    }
}

static inline bool CircularBuffer_Release(struct SolidSyslogBuffer* base)
{
    bool released = false;
    for (size_t i = 0; (i < SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE) && !released; i++)
    {
        SolidSyslog_LockConfig();
        if (Pool[i].InUse && CircularBuffer_SlotOwnsBase(&Pool[i], base))
        {
            CircularBuffer_Cleanup(base);
            Pool[i].InUse = false;
            released = true;
        }
        SolidSyslog_UnlockConfig();
    }
    return released;
}

static inline bool CircularBuffer_SlotOwnsBase(const struct Slot* slot, const struct SolidSyslogBuffer* base)
{
    return base == &slot->Object.Base;
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
