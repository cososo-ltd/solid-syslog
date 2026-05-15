#include "SolidSyslogAtomicCounter.h"

#include "SolidSyslogAtomicU32.h"

#include <stdbool.h>
#include <stddef.h>

struct SolidSyslogAtomicU32;

/* RFC 5424 §7.3.1: sequenceId MUST take values in [1, 2^31 - 1] and wrap to 1 (not 0) on overflow. */
static const uint32_t SEQUENCE_ID_MAX = 2147483647U;

struct SolidSyslogAtomicCounter
{
    SolidSyslogAtomicU32Storage Storage;
    struct SolidSyslogAtomicU32* Slot;
};

static struct SolidSyslogAtomicCounter instance;

static inline uint32_t AtomicCounter_NextSequenceId(uint32_t current)
{
    return (current >= SEQUENCE_ID_MAX) ? 1U : current + 1U;
}

static inline bool AtomicCounter_TryAdvance(struct SolidSyslogAtomicU32* slot, uint32_t* nextOut)
{
    uint32_t current = SolidSyslogAtomicU32_Load(slot);
    *nextOut = AtomicCounter_NextSequenceId(current);
    return SolidSyslogAtomicU32_CompareAndSwap(slot, current, *nextOut);
}

struct SolidSyslogAtomicCounter* SolidSyslogAtomicCounter_Create(void)
{
    instance.Slot = SolidSyslogAtomicU32_FromStorage(&instance.Storage);
    SolidSyslogAtomicU32_Init(instance.Slot, 0);
    return &instance;
}

void SolidSyslogAtomicCounter_Destroy(void)
{
    instance.Slot = NULL;
}

uint32_t SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter* counter)
{
    uint32_t next = 0;
    while (!AtomicCounter_TryAdvance(counter->Slot, &next))
    {
    }
    return next;
}
