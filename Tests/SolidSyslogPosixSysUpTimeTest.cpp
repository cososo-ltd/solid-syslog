#include <cstdint>

#include "ClockFake.h"
#include "SolidSyslogPosixSysUpTime.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogPosixSysUpTime)
{
    void setup() override
    {
        ClockFake_Reset();
    }
};

// clang-format on

TEST(SolidSyslogPosixSysUpTime, ZeroSecondsAndZeroNanosecondsReturnsZero)
{
    ClockFake_SetTime(0, 0);
    UNSIGNED_LONGS_EQUAL(0, SolidSyslogPosixSysUpTime_Get());
}

TEST(SolidSyslogPosixSysUpTime, OneSecondReturnsOneHundredHundredths)
{
    ClockFake_SetTime(1, 0);
    UNSIGNED_LONGS_EQUAL(100, SolidSyslogPosixSysUpTime_Get());
}

TEST(SolidSyslogPosixSysUpTime, TenMillisecondsIsOneHundredth)
{
    ClockFake_SetTime(0, 10000000);
    UNSIGNED_LONGS_EQUAL(1, SolidSyslogPosixSysUpTime_Get());
}

TEST(SolidSyslogPosixSysUpTime, SubHundredthNanosecondsTruncateToZero)
{
    ClockFake_SetTime(0, 9999999);
    UNSIGNED_LONGS_EQUAL(0, SolidSyslogPosixSysUpTime_Get());
}

TEST(SolidSyslogPosixSysUpTime, ClockGettimeFailureReturnsZero)
{
    ClockFake_SetTime(123, 0);
    ClockFake_SetClockGettimeReturn(-1);
    UNSIGNED_LONGS_EQUAL(0, SolidSyslogPosixSysUpTime_Get());
}

TEST(SolidSyslogPosixSysUpTime, MaxUint32Boundary)
{
    // 42949672 sec * 100 = 4,294,967,200; + 95 hundredths (950 ms) = 4,294,967,295 = UINT32_MAX
    ClockFake_SetTime(42949672, 950000000);
    UNSIGNED_LONGS_EQUAL(UINT32_MAX, SolidSyslogPosixSysUpTime_Get());
}

TEST(SolidSyslogPosixSysUpTime, WrapsPastMaxUint32)
{
    // 42949673 sec * 100 = 4,294,967,300 = UINT32_MAX + 5; uint32 cast wraps to 4
    ClockFake_SetTime(42949673, 0);
    UNSIGNED_LONGS_EQUAL(4, SolidSyslogPosixSysUpTime_Get());
}
