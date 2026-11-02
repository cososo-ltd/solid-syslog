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
