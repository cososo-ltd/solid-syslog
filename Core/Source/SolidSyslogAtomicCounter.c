#include "SolidSyslogAtomicCounter.h"

#include "SolidSyslogAtomicU32.h"

#include <stdbool.h>
#include <stddef.h>

/* RFC 5424 §7.3.1: sequenceId MUST take values in [1, 2^31 - 1] and wrap to 1 (not 0) on overflow. */
static const uint32_t SEQUENCE_ID_MAX = 2147483647U;

struct SolidSyslogAtomicCounter
{
    SolidSyslogAtomicU32Storage  storage;
    struct SolidSyslogAtomicU32* slot;
};

static struct SolidSyslogAtomicCounter instance;

static inline uint32_t AtomicCounter_NextSequenceId(uint32_t current)
{
    return (current >= SEQUENCE_ID_MAX) ? 1U : current + 1U;
}

static inline bool AtomicCounter_TryAdvance(struct SolidSyslogAtomicU32* slot, uint32_t* nextOut)
{
    uint32_t current = SolidSyslogAtomicU32_Load(slot);
    *nextOut         = AtomicCounter_NextSequenceId(current);
    return SolidSyslogAtomicU32_CompareAndSwap(slot, current, *nextOut);
}

struct SolidSyslogAtomicCounter* SolidSyslogAtomicCounter_Create(void)
{
    instance.slot = SolidSyslogAtomicU32_FromStorage(&instance.storage);
    SolidSyslogAtomicU32_Init(instance.slot, 0);
    return &instance;
}

void SolidSyslogAtomicCounter_Destroy(void)
{
    instance.slot = NULL;
}

uint32_t SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter* counter)
{
    uint32_t next = 0;
    while (!AtomicCounter_TryAdvance(counter->slot, &next))
    {
    }
    return next;
}
