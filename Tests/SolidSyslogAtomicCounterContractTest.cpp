#include "CppUTest/TestHarness.h"

#include <stdint.h>

extern "C"
{
#include "SolidSyslogAtomicCounterTestHelper.h"
}

TEST_GROUP(AtomicCounterContract){};

TEST(AtomicCounterContract, CreateReturnsNonNullHandle)
{
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create();

    CHECK(counter != nullptr);

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, FirstIncrementReturnsOne)
{
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create();

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, SequentialIncrementsCount1Then2Then3)
{
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create();

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counter));
    LONGS_EQUAL(2, TestAtomicCounter_Increment(counter));
    LONGS_EQUAL(3, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

/* Independence of two simultaneous counters is only observable when the
 * runtime pool can host at least two slots. The default build runs at
 * pool size 1; the `tunable-override-debug` preset bumps it to 2 (see
 * Tests/Fixtures/TunableOverrides.h). When pool size < 2, print
 * a notice and exit cleanly via TEST_EXIT so the test is honestly
 * accounted for. */
TEST(AtomicCounterContract, TwoCountersFromPoolAreIndependent)
{
    if (TestAtomicCounter_PoolSize() < 2U)
    {
        UT_PRINT("Pool size < 2 — counter independence only observable under tunable-override-debug");
        TEST_EXIT;
    }

    struct SolidSyslogAtomicCounter* counterA = TestAtomicCounter_Create();
    struct SolidSyslogAtomicCounter* counterB = TestAtomicCounter_Create();

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counterA));
    LONGS_EQUAL(2, TestAtomicCounter_Increment(counterA));
    LONGS_EQUAL(1, TestAtomicCounter_Increment(counterB));
    LONGS_EQUAL(3, TestAtomicCounter_Increment(counterA));
    LONGS_EQUAL(2, TestAtomicCounter_Increment(counterB));

    TestAtomicCounter_Destroy(counterA);
    TestAtomicCounter_Destroy(counterB);
}

TEST(AtomicCounterContract, IncrementAfterInitReturnsValuePlusOne)
{
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create();

    TestAtomicCounter_Init(counter, 5U);

    LONGS_EQUAL(6, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, IncrementJustBelowMaxReturnsMax)
{
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create();

    /* The step before the wrap must not wrap: INT32_MAX - 1 -> INT32_MAX. */
    TestAtomicCounter_Init(counter, (uint32_t) INT32_MAX - 1U);

    LONGS_EQUAL(INT32_MAX, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, IncrementAtMaxWrapsToOne)
{
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create();

    /* RFC 5424 §7.3.1: sequenceId values are in [1, 2^31 - 1] and wrap to 1, not 0. */
    TestAtomicCounter_Init(counter, (uint32_t) INT32_MAX);

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}
