#include "CppUTest/TestHarness.h"
#include "SolidSyslogWindowsSysUpTime.h"
#include "SolidSyslogWindowsSysUpTimeInternal.h"

#include <cstdint>
#include <windows.h>

static ULONGLONG fakeTickCount;

static ULONGLONG WINAPI FakeGetTickCount64(void)
{
    return fakeTickCount;
}

// clang-format off
TEST_GROUP(SolidSyslogWindowsSysUpTime)
{
    void setup() override
    {
        fakeTickCount = 0;
        UT_PTR_SET(WindowsSysUpTime_GetTickCount64, FakeGetTickCount64);
    }
};

// clang-format on

TEST(SolidSyslogWindowsSysUpTime, ZeroMillisecondsReturnsZero)
{
    fakeTickCount = 0;
    UNSIGNED_LONGS_EQUAL(0, SolidSyslogWindowsSysUpTime_Get());
}

TEST(SolidSyslogWindowsSysUpTime, TenMillisecondsIsOneHundredth)
{
    fakeTickCount = 10;
    UNSIGNED_LONGS_EQUAL(1, SolidSyslogWindowsSysUpTime_Get());
}

TEST(SolidSyslogWindowsSysUpTime, NineMillisecondsTruncatesToZero)
{
    fakeTickCount = 9;
    UNSIGNED_LONGS_EQUAL(0, SolidSyslogWindowsSysUpTime_Get());
}

TEST(SolidSyslogWindowsSysUpTime, OneSecondIsOneHundredHundredths)
{
    fakeTickCount = 1000;
    UNSIGNED_LONGS_EQUAL(100, SolidSyslogWindowsSysUpTime_Get());
}

TEST(SolidSyslogWindowsSysUpTime, MaxUint32Boundary)
{
    // UINT32_MAX hundredths = 4,294,967,295 hundredths * 10 ms/hundredth = 42,949,672,950 ms
    fakeTickCount = 42949672950ULL;
    UNSIGNED_LONGS_EQUAL(UINT32_MAX, SolidSyslogWindowsSysUpTime_Get());
}

TEST(SolidSyslogWindowsSysUpTime, WrapsPastMaxUint32)
{
    // 42,949,672,970 ms / 10 = 4,294,967,297 hundredths = UINT32_MAX + 2 → wraps to 1
    fakeTickCount = 42949672970ULL;
    UNSIGNED_LONGS_EQUAL(1, SolidSyslogWindowsSysUpTime_Get());
}
