#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogVxWorks64Sleep.h"
#include "VxWorks64TaskFake.h"

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

    LONGS_EQUAL(60, VxWorks64TaskFake_LastDelayTicks());
}

TEST(SolidSyslogVxWorks64Sleep, APartTickRoundsUpToAWholeOne)
{
    SolidSyslogVxWorks64_Sleep(1);

    LONGS_EQUAL(1, VxWorks64TaskFake_LastDelayTicks());
}
