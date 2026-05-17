#include "SolidSyslogAtomicCounterTestHelper.h"

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogWindowsAtomicCounter.h"

#include <stdint.h>

struct SolidSyslogAtomicCounter;

/* Whitebox include: SolidSyslogWindowsAtomicCounter.c is compiled into this
   test translation unit so the static WindowsAtomicCounter_Init helper is
   directly reachable for test-only setup. The library's own
   SolidSyslogWindowsAtomicCounter.o is then not pulled in by the linker
   (static-archive object-on-demand resolution), so there is no
   duplicate-symbol conflict. */
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "SolidSyslogWindowsAtomicCounter.c"

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(SolidSyslogWindowsAtomicCounterStorage) <= TEST_ATOMIC_COUNTER_STORAGE_SIZE,
    TestAtomicCounterStorage_too_small_for_Windows
);

struct SolidSyslogAtomicCounter* TestAtomicCounter_Create(TestAtomicCounterStorage* storage)
{
    return SolidSyslogWindowsAtomicCounter_Create((SolidSyslogWindowsAtomicCounterStorage*) storage);
}

void TestAtomicCounter_Init(struct SolidSyslogAtomicCounter* base, uint32_t value)
{
    WindowsAtomicCounter_Init(WindowsAtomicCounter_SelfFromBase(base), value);
}

uint32_t TestAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    return SolidSyslogAtomicCounter_Increment(base);
}

void TestAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    SolidSyslogWindowsAtomicCounter_Destroy(base);
}
