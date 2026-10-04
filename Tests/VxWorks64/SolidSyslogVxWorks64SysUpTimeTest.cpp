#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogVxWorks64SysUpTime.h"
#include "VxWorks64TaskFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64SysUpTime)
{
    void setup() override
    {
        VxWorks64TaskFake_Reset();
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64SysUpTime, ZeroTicksIsZeroUptime)
{
    UNSIGNED_LONGS_EQUAL(0, SolidSyslogVxWorks64_GetSysUpTime());
}

TEST(SolidSyslogVxWorks64SysUpTime, OneSecondOfTicksIsOneHundredHundredths)
{
    VxWorks64TaskFake_SetTicks(60U);

    UNSIGNED_LONGS_EQUAL(100, SolidSyslogVxWorks64_GetSysUpTime());
}
