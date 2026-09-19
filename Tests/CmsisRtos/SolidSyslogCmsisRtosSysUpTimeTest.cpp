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
