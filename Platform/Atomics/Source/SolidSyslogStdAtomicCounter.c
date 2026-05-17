#include "SolidSyslogStdAtomicCounter.h"

#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogMacros.h"

#include <stdatomic.h>
#include <stddef.h>

struct SolidSyslogStdAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    _Atomic uint32_t Value;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogStdAtomicCounter) <= sizeof(SolidSyslogStdAtomicCounterStorage),
    SolidSyslogStdAtomicCounterStorage_too_small
);

static uint32_t StdAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);
static void StdAtomicCounter_Init(struct SolidSyslogStdAtomicCounter* self, uint32_t value);
static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromBase(struct SolidSyslogAtomicCounter* base);
static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromStorage(
    SolidSyslogStdAtomicCounterStorage* storage
);

struct SolidSyslogAtomicCounter* SolidSyslogStdAtomicCounter_Create(SolidSyslogStdAtomicCounterStorage* storage)
{
    struct SolidSyslogStdAtomicCounter* self = StdAtomicCounter_SelfFromStorage(storage);
    self->Base.Increment = StdAtomicCounter_Increment;
    StdAtomicCounter_Init(self, 0U);
    return &self->Base;
}

void SolidSyslogStdAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogStdAtomicCounter* self = StdAtomicCounter_SelfFromBase(base);
    self->Base.Increment = NULL;
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

static void StdAtomicCounter_Init(struct SolidSyslogStdAtomicCounter* self, uint32_t value)
{
    atomic_init(&self->Value, value);
}

static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromBase(struct SolidSyslogAtomicCounter* base)
{
    return (struct SolidSyslogStdAtomicCounter*) base;
}

static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromStorage(
    SolidSyslogStdAtomicCounterStorage* storage
)
{
    return (struct SolidSyslogStdAtomicCounter*) storage;
}
