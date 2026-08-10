#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "FreeRtosTaskFake.h"
#include "SolidSyslogFreeRtosSysUpTime.h"
#include "FreeRTOS.h"

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosSysUpTime)
{
    void setup() override
    {
        FreeRtosTaskFake_Reset();
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosSysUpTime, ReturnsZeroWhenTicksAreZero)

{
    FreeRtosTaskFake_SetTickCount(0);

    UNSIGNED_LONGS_EQUAL(0U, SolidSyslogFreeRtos_GetSysUpTime());
}

TEST(SolidSyslogFreeRtosSysUpTime, ReturnsOneWhenTicksAreOne)

{
    FreeRtosTaskFake_SetTickCount(1);

    UNSIGNED_LONGS_EQUAL(1U, SolidSyslogFreeRtos_GetSysUpTime());
}

TEST(SolidSyslogFreeRtosSysUpTime, ReturnsTickCountAtMidRange)

{
    FreeRtosTaskFake_SetTickCount(12345U);

    UNSIGNED_LONGS_EQUAL(12345U, SolidSyslogFreeRtos_GetSysUpTime());
}

TEST(SolidSyslogFreeRtosSysUpTime, ReturnsUint32MaxWhenTicksAreUint32Max)

{
    FreeRtosTaskFake_SetTickCount(UINT32_MAX);

    UNSIGNED_LONGS_EQUAL(UINT32_MAX, SolidSyslogFreeRtos_GetSysUpTime());
}
