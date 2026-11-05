#include "CounterFake.h"
#include "SolidSyslogAtomicCounter.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(CounterFake){};

TEST(CounterFake, CountsFromOne)
{
    struct SolidSyslogAtomicCounter* counter = CounterFake_Create();
    LONGS_EQUAL(1, SolidSyslogAtomicCounter_Increment(counter));
    LONGS_EQUAL(2, SolidSyslogAtomicCounter_Increment(counter));
    CounterFake_Destroy(counter);
}

TEST(CounterFake, NothingIsHandedOutBeforeTheFirstIncrement)
{
    struct SolidSyslogAtomicCounter* counter = CounterFake_Create();
    LONGS_EQUAL(0, CounterFake_LastHandedOut(counter));
    CounterFake_Destroy(counter);
}

TEST(CounterFake, TheLastNumberHandedOutIsRead)
{
    struct SolidSyslogAtomicCounter* counter = CounterFake_Create();
    (void) SolidSyslogAtomicCounter_Increment(counter);
    (void) SolidSyslogAtomicCounter_Increment(counter);
    LONGS_EQUAL(2, CounterFake_LastHandedOut(counter));
    CounterFake_Destroy(counter);
}
