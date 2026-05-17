#include "SolidSyslogAtomicCounterTestHelper.h"

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogStdAtomicCounter.h"

#include <stdint.h>

struct SolidSyslogAtomicCounter;

/* Whitebox include: SolidSyslogStdAtomicCounter.c is compiled into this test
   translation unit so the static StdAtomicCounter_Init helper is directly
   reachable for test-only setup (e.g. positioning the counter near
   INT32_MAX for the wraparound test). The library's own
   SolidSyslogStdAtomicCounter.o is then not pulled in by the linker
   (static-archive object-on-demand resolution), so there is no
   duplicate-symbol conflict. */
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "SolidSyslogStdAtomicCounter.c"

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(SolidSyslogStdAtomicCounterStorage) <= TEST_ATOMIC_COUNTER_STORAGE_SIZE,
    TestAtomicCounterStorage_too_small_for_Std
);

struct SolidSyslogAtomicCounter* TestAtomicCounter_Create(TestAtomicCounterStorage* storage)
{
    return SolidSyslogStdAtomicCounter_Create((SolidSyslogStdAtomicCounterStorage*) storage);
}

void TestAtomicCounter_Init(struct SolidSyslogAtomicCounter* base, uint32_t value)
{
    StdAtomicCounter_Init(StdAtomicCounter_SelfFromBase(base), value);
}

uint32_t TestAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    return SolidSyslogAtomicCounter_Increment(base);
}

void TestAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    SolidSyslogStdAtomicCounter_Destroy(base);
}
