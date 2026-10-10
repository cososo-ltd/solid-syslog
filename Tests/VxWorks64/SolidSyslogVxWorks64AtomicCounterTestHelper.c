#include "SolidSyslogAtomicCounterTestHelper.h"

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64AtomicCounter.h"

#include <stddef.h>
#include <stdint.h>

struct SolidSyslogAtomicCounter;

/* Whitebox include: SolidSyslogVxWorks64AtomicCounter.c is compiled into this
   test translation unit so the static VxWorks64AtomicCounter_Init helper is
   directly reachable for test-only setup. The executable lists only the Static
   source beside it, so the counter is compiled once. */
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "SolidSyslogVxWorks64AtomicCounter.c"

struct SolidSyslogAtomicCounter* TestAtomicCounter_Create(void)
{
    return SolidSyslogVxWorks64AtomicCounter_Create();
}

void TestAtomicCounter_Init(struct SolidSyslogAtomicCounter* base, uint32_t value)
{
    VxWorks64AtomicCounter_Init(VxWorks64AtomicCounter_SelfFromBase(base), value);
}

uint32_t TestAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    return SolidSyslogAtomicCounter_Increment(base);
}

void TestAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    SolidSyslogVxWorks64AtomicCounter_Destroy(base);
}

size_t TestAtomicCounter_PoolSize(void)
{
    return SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE;
}
