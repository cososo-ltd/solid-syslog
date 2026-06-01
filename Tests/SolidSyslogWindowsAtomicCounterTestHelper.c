#include "SolidSyslogAtomicCounterTestHelper.h"

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsAtomicCounter.h"

#include <stddef.h>
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

struct SolidSyslogAtomicCounter* TestAtomicCounter_Create(void)
{
    return SolidSyslogWindowsAtomicCounter_Create();
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

size_t TestAtomicCounter_PoolSize(void)
{
    return SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE;
}
