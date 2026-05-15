#include "SolidSyslogAtomicU32.h"

#include "SolidSyslogMacros.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

struct SolidSyslogAtomicU32
{
    _Atomic uint32_t Value;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogAtomicU32) <= sizeof(SolidSyslogAtomicU32Storage),
    SolidSyslogAtomicU32Storage_too_small_for_StdAtomicU32
);
SOLIDSYSLOG_STATIC_ASSERT(
    _Alignof(struct SolidSyslogAtomicU32) <= _Alignof(SolidSyslogAtomicU32Storage),
    SolidSyslogAtomicU32Storage_misaligned_for_StdAtomicU32
);

struct SolidSyslogAtomicU32* SolidSyslogAtomicU32_FromStorage(SolidSyslogAtomicU32Storage* storage)
{
    uint8_t* bytes = (uint8_t*) storage;
    return (struct SolidSyslogAtomicU32*) bytes;
}

void SolidSyslogAtomicU32_Init(struct SolidSyslogAtomicU32* slot, uint32_t value)
{
    atomic_init(&slot->Value, value);
}

uint32_t SolidSyslogAtomicU32_Load(struct SolidSyslogAtomicU32* slot)
{
    return atomic_load_explicit(&slot->Value, memory_order_relaxed);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — CAS shape is universal (compare_exchange convention)
bool SolidSyslogAtomicU32_CompareAndSwap(struct SolidSyslogAtomicU32* slot, uint32_t expected, uint32_t desired)
{
    uint32_t expectedLocal = expected;
    return atomic_compare_exchange_strong_explicit(
        &slot->Value,
        &expectedLocal,
        desired,
        memory_order_relaxed,
        memory_order_relaxed
    );
}
