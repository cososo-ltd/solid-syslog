#include "SolidSyslogStdAtomicOps.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogAtomicOpsDefinition.h"

struct SolidSyslogStdAtomicOps
{
    struct SolidSyslogAtomicOps base;
    atomic_uint_least32_t       value;
};

static uint32_t Load(struct SolidSyslogAtomicOps* self);
static bool     CompareAndSwap(struct SolidSyslogAtomicOps* self, uint32_t expected, uint32_t desired);

static struct SolidSyslogStdAtomicOps instance;

struct SolidSyslogAtomicOps* SolidSyslogStdAtomicOps_Create(void)
{
    instance.base.Load           = Load;
    instance.base.CompareAndSwap = CompareAndSwap;
    atomic_store_explicit(&instance.value, 0, memory_order_relaxed);
    return &instance.base;
}

void SolidSyslogStdAtomicOps_Destroy(void)
{
    instance.base.Load           = NULL;
    instance.base.CompareAndSwap = NULL;
    atomic_store_explicit(&instance.value, 0, memory_order_relaxed);
}

static uint32_t Load(struct SolidSyslogAtomicOps* self)
{
    struct SolidSyslogStdAtomicOps* std = (struct SolidSyslogStdAtomicOps*) self;
    return (uint32_t) atomic_load_explicit(&std->value, memory_order_relaxed);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — CAS shape is universal (compare_exchange convention)
static bool CompareAndSwap(struct SolidSyslogAtomicOps* self, uint32_t expected, uint32_t desired)
{
    struct SolidSyslogStdAtomicOps* std        = (struct SolidSyslogStdAtomicOps*) self;
    uint_least32_t                  expectedAt = expected;
    return atomic_compare_exchange_strong_explicit(&std->value, &expectedAt, desired, memory_order_relaxed, memory_order_relaxed);
}
