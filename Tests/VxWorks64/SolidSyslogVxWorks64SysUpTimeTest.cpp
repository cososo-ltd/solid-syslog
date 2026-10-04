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

TEST(SolidSyslogVxWorks64SysUpTime, TicksBeyondThirtyTwoBitsScaleBeforeTruncating)
{
    // 2^32 ticks at 60 Hz is 7158278826 hundredths, which leaves 0xAAAAAAAA
    // once taken modulo 2^32.
    VxWorks64TaskFake_SetTicks(0x100000000ULL);

    UNSIGNED_LONGS_EQUAL(0xAAAAAAAAUL, SolidSyslogVxWorks64_GetSysUpTime());
}

TEST(SolidSyslogVxWorks64SysUpTime, WrapsAtTwoToTheThirtyTwoHundredths)
{
    // 2576980381 ticks at 60 Hz is 2^32 + 5 hundredths: about 497 days.
    VxWorks64TaskFake_SetTicks(2576980381ULL);

    UNSIGNED_LONGS_EQUAL(5, SolidSyslogVxWorks64_GetSysUpTime());
}
