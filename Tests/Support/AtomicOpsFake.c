#include "AtomicOpsFake.h"

#include <stdbool.h>

#include "SolidSyslogAtomicOpsDefinition.h"

static uint32_t loadValue;
static bool     nextCompareAndSwapShouldFail;
static uint32_t loadShiftValue;

static uint32_t Load(struct SolidSyslogAtomicOps* self)
{
    (void) self;
    return loadValue;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — CAS shape is universal (compare_exchange convention)
static bool CompareAndSwap(struct SolidSyslogAtomicOps* self, uint32_t expected, uint32_t desired)
{
    (void) self;
    if (nextCompareAndSwapShouldFail)
    {
        nextCompareAndSwapShouldFail = false;
        loadValue                    = loadShiftValue;
        return false;
    }
    if (loadValue != expected)
    {
        return false;
    }
    loadValue = desired;
    return true;
}

static struct SolidSyslogAtomicOps instance = {
    .Load           = Load,
    .CompareAndSwap = CompareAndSwap,
};

void AtomicOpsFake_Reset(void)
{
    loadValue                    = 0;
    nextCompareAndSwapShouldFail = false;
    loadShiftValue               = 0;
}

struct SolidSyslogAtomicOps* AtomicOpsFake_Get(void)
{
    return &instance;
}

void AtomicOpsFake_SetLoadValue(uint32_t value)
{
    loadValue = value;
}

void AtomicOpsFake_FailNextCompareAndSwapAndShiftLoadTo(uint32_t value)
{
    nextCompareAndSwapShouldFail = true;
    loadShiftValue               = value;
}
