#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <climits>

#include "SolidSyslogVxWorks64Sleep.h"
#include "VxWorks64TaskFake.h"

// Asserts the sleep made exactly one taskDelay, for this many ticks.
#define CHECK_DELAYED_FOR(ticks)                                  \
    {                                                             \
        UNSIGNED_LONGS_EQUAL(1, VxWorks64TaskFake_DelayCount());  \
        LONGS_EQUAL((ticks), VxWorks64TaskFake_LastDelayTicks()); \
    }

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Sleep)
{
    void setup() override
    {
        VxWorks64TaskFake_Reset();
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Sleep, OneSecondIsOneSecondOfTicks)
{
    SolidSyslogVxWorks64_Sleep(1000);

    CHECK_DELAYED_FOR(60);
}

TEST(SolidSyslogVxWorks64Sleep, APartTickRoundsUpToAWholeOne)
{
    SolidSyslogVxWorks64_Sleep(1);

    CHECK_DELAYED_FOR(1);
}

TEST(SolidSyslogVxWorks64Sleep, TicksAreReckonedInSixtyFourBits)
{
    // INT_MAX ms at 60 Hz is 128849018.82 ticks, rounded up; reckoned in int
    // the product would overflow.
    SolidSyslogVxWorks64_Sleep(INT_MAX);

    CHECK_DELAYED_FOR(128849019);
}

TEST(SolidSyslogVxWorks64Sleep, ANegativeSleepOnlyYields)
{
    SolidSyslogVxWorks64_Sleep(-100);

    CHECK_DELAYED_FOR(0);
}

TEST(SolidSyslogVxWorks64Sleep, AZeroSleepYieldsForNoTicks)
{
    SolidSyslogVxWorks64_Sleep(0);

    CHECK_DELAYED_FOR(0);
}
