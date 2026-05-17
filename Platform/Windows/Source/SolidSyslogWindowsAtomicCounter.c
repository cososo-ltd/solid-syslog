#include "SolidSyslogWindowsAtomicCounter.h"

#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogMacros.h"

#include <stddef.h>
#include <windows.h>

struct SolidSyslogWindowsAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    volatile LONG Value;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogWindowsAtomicCounter) <= sizeof(SolidSyslogWindowsAtomicCounterStorage),
    SolidSyslogWindowsAtomicCounterStorage_too_small
);

static uint32_t WindowsAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);
static void WindowsAtomicCounter_Init(struct SolidSyslogWindowsAtomicCounter* self, uint32_t value);
static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
);
static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromStorage(
    SolidSyslogWindowsAtomicCounterStorage* storage
);

struct SolidSyslogAtomicCounter* SolidSyslogWindowsAtomicCounter_Create(SolidSyslogWindowsAtomicCounterStorage* storage)
{
    struct SolidSyslogWindowsAtomicCounter* self = WindowsAtomicCounter_SelfFromStorage(storage);
    self->Base.Increment = WindowsAtomicCounter_Increment;
    WindowsAtomicCounter_Init(self, 0U);
    return &self->Base;
}

void SolidSyslogWindowsAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogWindowsAtomicCounter* self = WindowsAtomicCounter_SelfFromBase(base);
    self->Base.Increment = NULL;
}

static uint32_t WindowsAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogWindowsAtomicCounter* self = WindowsAtomicCounter_SelfFromBase(base);
    LONG current = InterlockedCompareExchange(&self->Value, 0, 0);
    LONG next = 0;
    LONG previous = 0;
    do
    {
        next = ((uint32_t) current >= SOLIDSYSLOG_SEQUENCE_ID_MAX) ? 1 : (current + 1);
        previous = InterlockedCompareExchange(&self->Value, next, current);
        if (previous == current)
        {
            break;
        }
        current = previous;
    } while (1);
    return (uint32_t) next;
}

static void WindowsAtomicCounter_Init(struct SolidSyslogWindowsAtomicCounter* self, uint32_t value)
{
    self->Value = (LONG) value;
}

static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
)
{
    return (struct SolidSyslogWindowsAtomicCounter*) base;
}

static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromStorage(
    SolidSyslogWindowsAtomicCounterStorage* storage
)
{
    return (struct SolidSyslogWindowsAtomicCounter*) storage;
}
