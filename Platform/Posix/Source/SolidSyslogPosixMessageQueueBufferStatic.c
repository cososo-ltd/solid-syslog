#include "SolidSyslogPosixMessageQueueBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"
#include "SolidSyslogPosixMessageQueueBufferPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogBuffer;

/* Each pool slot's queue name is `/solidsyslog_<pid>_<slotIndex>`. The
 * pid keeps the name unique per process; the slot index keeps multiple
 * in-process pool entries from aliasing onto the same kernel queue
 * object when the pool tunable is bumped above 1. */

static inline size_t PosixMessageQueueBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base);
static inline void PosixMessageQueueBuffer_CleanupAtIndex(size_t index, void* context);

static bool PosixMessageQueueBuffer_InUse[SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE];
static struct SolidSyslogPosixMessageQueueBuffer
    PosixMessageQueueBuffer_Pool[SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixMessageQueueBuffer_Allocator = {
    PosixMessageQueueBuffer_InUse,
    SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- distinct semantic meaning; struct wrapper would over-engineer
struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(size_t maxMessageSize, long maxMessages)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixMessageQueueBuffer_Allocator);
    struct SolidSyslogBuffer* handle = SolidSyslogNullBuffer_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixMessageQueueBuffer_Allocator, index) == true)
    {
        bool opened = PosixMessageQueueBuffer_Initialise(
            &PosixMessageQueueBuffer_Pool[index].Base,
            maxMessageSize,
            maxMessages,
            index
        );
        if (opened)
        {
            handle = &PosixMessageQueueBuffer_Pool[index].Base;
        }
        else
        {
            (void) SolidSyslogPoolAllocator_FreeIfInUse(
                &PosixMessageQueueBuffer_Allocator,
                index,
                PosixMessageQueueBuffer_CleanupAtIndex,
                NULL
            );
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &PosixMessageQueueBufferErrorSource,
                (uint8_t) POSIXMESSAGEQUEUEBUFFER_ERROR_MQ_OPEN_FAILED
            );
        }
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PosixMessageQueueBufferErrorSource,
            (uint8_t) POSIXMESSAGEQUEUEBUFFER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPosixMessageQueueBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    size_t index = PosixMessageQueueBuffer_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&PosixMessageQueueBuffer_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &PosixMessageQueueBuffer_Allocator,
                        index,
                        PosixMessageQueueBuffer_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &PosixMessageQueueBufferErrorSource,
            (uint8_t) POSIXMESSAGEQUEUEBUFFER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PosixMessageQueueBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base)
{
    size_t result = SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE; poolIndex++)
    {
        if (base == &PosixMessageQueueBuffer_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixMessageQueueBuffer_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixMessageQueueBuffer_Cleanup(&PosixMessageQueueBuffer_Pool[index].Base);
}
