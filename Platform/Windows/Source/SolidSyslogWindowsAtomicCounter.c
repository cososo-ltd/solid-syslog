#include "SolidSyslogWindowsAtomicCounter.h"

#include <stdint.h>
#include <windows.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogWindowsAtomicCounterPrivate.h"

static uint32_t WindowsAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);

static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
);

void WindowsAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogWindowsAtomicCounter* self = WindowsAtomicCounter_SelfFromBase(base);
    self->Base.Increment = WindowsAtomicCounter_Increment;
    WindowsAtomicCounter_Init(self, 0U);
}

static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
)
{
    return (struct SolidSyslogWindowsAtomicCounter*) base;
}

void WindowsAtomicCounter_Init(struct SolidSyslogWindowsAtomicCounter* self, uint32_t value)
{
    self->Value = (LONG) value;
}

void WindowsAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base)
{
    /* Overwrite the abstract base with the shared NullAtomicCounter vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullAtomicCounter_Get();
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
