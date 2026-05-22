#include "SolidSyslogStdAtomicCounter.h"

#include <stdatomic.h>
#include <stdint.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogStdAtomicCounterPrivate.h"

static uint32_t StdAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);

static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromBase(struct SolidSyslogAtomicCounter* base);

void StdAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogStdAtomicCounter* self = StdAtomicCounter_SelfFromBase(base);
    self->Base.Increment = StdAtomicCounter_Increment;
    StdAtomicCounter_Init(self, 0U);
}

static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromBase(struct SolidSyslogAtomicCounter* base)
{
    return (struct SolidSyslogStdAtomicCounter*) base;
}

void StdAtomicCounter_Init(struct SolidSyslogStdAtomicCounter* self, uint32_t value)
{
    atomic_init(&self->Value, value);
}

void StdAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base)
{
    /* Overwrite the abstract base with the shared NullAtomicCounter vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullAtomicCounter_Get();
}

static uint32_t StdAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogStdAtomicCounter* self = StdAtomicCounter_SelfFromBase(base);
    uint32_t current = atomic_load_explicit(&self->Value, memory_order_relaxed);
    uint32_t next = 0U;
    do
    {
        next = (current >= SOLIDSYSLOG_SEQUENCE_ID_MAX) ? 1U : (current + 1U);
    } while (!atomic_compare_exchange_strong_explicit(
        &self->Value,
        &current,
        next,
        memory_order_relaxed,
        memory_order_relaxed
    ));
    return next;
}
