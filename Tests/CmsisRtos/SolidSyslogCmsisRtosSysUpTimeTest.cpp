#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "CmsisRtosKernelFake.h"
#include "SolidSyslogCmsisRtosSysUpTime.h"
#include "SolidSyslogCmsisRtosSysUpTimeTestHelper.h"

// clang-format off
TEST_GROUP(SolidSyslogCmsisRtosSysUpTime)
{
    void setup() override
    {
        CmsisRtosKernelFake_Reset();
        TestCmsisRtosSysUpTime_Reset();
    }

    [[nodiscard]] static uint32_t uptimeAt(uint32_t ticks)
    {
        CmsisRtosKernelFake_SetTickCount(ticks);
        return SolidSyslogCmsisRtos_GetSysUpTime();
    }
};

// clang-format on

TEST(SolidSyslogCmsisRtosSysUpTime, ReturnsZeroWhenTicksAreZero)
{
    CmsisRtosKernelFake_SetTickCount(0);

    UNSIGNED_LONGS_EQUAL(0U, SolidSyslogCmsisRtos_GetSysUpTime());
}

TEST(SolidSyslogCmsisRtosSysUpTime, ReturnsOneHundredthForOneTickAtOneHundredHertz)
{
    CmsisRtosKernelFake_SetTickFreq(100);
    CmsisRtosKernelFake_SetTickCount(1);

    UNSIGNED_LONGS_EQUAL(1U, SolidSyslogCmsisRtos_GetSysUpTime());
}

TEST(SolidSyslogCmsisRtosSysUpTime, ScalesByTheKernelTickFrequency)
{
    CmsisRtosKernelFake_SetTickFreq(1000);
    CmsisRtosKernelFake_SetTickCount(1000);

    UNSIGNED_LONGS_EQUAL(100U, SolidSyslogCmsisRtos_GetSysUpTime());
}

TEST(SolidSyslogCmsisRtosSysUpTime, ScalesALargeTickCountWithoutOverflowing)
{
    CmsisRtosKernelFake_SetTickFreq(1000);
    CmsisRtosKernelFake_SetTickCount(100000000);

    UNSIGNED_LONGS_EQUAL(10000000U, SolidSyslogCmsisRtos_GetSysUpTime());
}

TEST(SolidSyslogCmsisRtosSysUpTime, KeepsCountingPastTheCounterWrap)
{
    CmsisRtosKernelFake_SetTickFreq(1000);

    uint32_t before = uptimeAt(UINT32_MAX);
    uint32_t after = uptimeAt(10000);

    // 10001 ticks on from UINT32_MAX, which is 1000 hundredths at 1000 Hz.
    UNSIGNED_LONGS_EQUAL(1000U, after - before);
}
