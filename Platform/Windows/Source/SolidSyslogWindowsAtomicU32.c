#include "SolidSyslogAtomicU32.h"

#include "SolidSyslogMacros.h"

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

struct SolidSyslogAtomicU32
{
    volatile LONG Value;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogAtomicU32) <= sizeof(SolidSyslogAtomicU32Storage),
    SolidSyslogAtomicU32Storage_too_small_for_WindowsAtomicU32
);
SOLIDSYSLOG_STATIC_ASSERT(
    _Alignof(struct SolidSyslogAtomicU32) <= _Alignof(SolidSyslogAtomicU32Storage),
    SolidSyslogAtomicU32Storage_misaligned_for_WindowsAtomicU32
);

struct SolidSyslogAtomicU32* SolidSyslogAtomicU32_FromStorage(SolidSyslogAtomicU32Storage* storage)
{
    uint8_t* bytes = (uint8_t*) storage;
    return (struct SolidSyslogAtomicU32*) bytes;
}

void SolidSyslogAtomicU32_Init(struct SolidSyslogAtomicU32* slot, uint32_t value)
{
    slot->Value = (LONG) value;
}

uint32_t SolidSyslogAtomicU32_Load(struct SolidSyslogAtomicU32* slot)
{
    return (uint32_t) InterlockedCompareExchange(&slot->Value, 0, 0);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — CAS shape is universal (compare_exchange convention)
bool SolidSyslogAtomicU32_CompareAndSwap(struct SolidSyslogAtomicU32* slot, uint32_t expected, uint32_t desired)
{
    LONG actual = InterlockedCompareExchange(&slot->Value, (LONG) desired, (LONG) expected);
    return (LONG) expected == actual;
}
