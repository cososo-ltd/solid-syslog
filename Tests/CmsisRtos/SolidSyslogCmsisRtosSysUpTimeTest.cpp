#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "CmsisRtosKernelFake.h"
#include "SolidSyslogCmsisRtosSysUpTime.h"

// clang-format off
TEST_GROUP(SolidSyslogCmsisRtosSysUpTime)
{
    void setup() override
    {
        CmsisRtosKernelFake_Reset();
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
