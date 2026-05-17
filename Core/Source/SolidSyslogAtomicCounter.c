#include "SolidSyslogAtomicCounter.h"

#include "SolidSyslogAtomicU32.h"

#include <stdbool.h>
#include <stddef.h>

struct SolidSyslogAtomicU32;

struct SolidSyslogAtomicCounter
{
    SolidSyslogAtomicU32Storage Storage;
    struct SolidSyslogAtomicU32* Slot;
};

static struct SolidSyslogAtomicCounter AtomicCounter_Instance;

static inline uint32_t AtomicCounter_NextSequenceId(uint32_t current)
{
    /* RFC 5424 §7.3.1: sequenceId MUST take values in [1, 2^31 - 1] and wrap to 1 (not 0) on overflow. */
    static const uint32_t sequenceIdMax = 2147483647U;
    return (current >= sequenceIdMax) ? 1U : (current + 1U);
}

static inline bool AtomicCounter_TryAdvance(struct SolidSyslogAtomicU32* slot, uint32_t* nextOut)
{
    uint32_t current = SolidSyslogAtomicU32_Load(slot);
    *nextOut = AtomicCounter_NextSequenceId(current);
    return SolidSyslogAtomicU32_CompareAndSwap(slot, current, *nextOut);
}

struct SolidSyslogAtomicCounter* SolidSyslogAtomicCounter_Create(void)
{
    AtomicCounter_Instance.Slot = SolidSyslogAtomicU32_FromStorage(&AtomicCounter_Instance.Storage);
    SolidSyslogAtomicU32_Init(AtomicCounter_Instance.Slot, 0);
    return &AtomicCounter_Instance;
}

void SolidSyslogAtomicCounter_Destroy(void)
{
    AtomicCounter_Instance.Slot = NULL;
}

uint32_t SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter* counter)
{
    uint32_t next = 0;
    while (!AtomicCounter_TryAdvance(counter->Slot, &next))
    {
    }
    return next;
}
