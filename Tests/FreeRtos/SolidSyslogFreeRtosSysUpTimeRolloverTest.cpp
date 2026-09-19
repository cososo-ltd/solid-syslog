/* Compiled at 1000 Hz, where a 32-bit tick counter wraps ten times sooner
 * than the 2^32 hundredths RFC 3418 allows. At the 100 Hz the other target
 * uses, ticks are hundredths and the two wraps coincide, so nothing here can
 * be observed there.
 *
 * The counted wraps live in a file-scope static with no way to reset it, so
 * every assertion is on a difference rather than an absolute. */
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "FreeRTOS.h"
#include "FreeRtosTaskFake.h"
#include "SolidSyslogFreeRtosSysUpTime.h"

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosSysUpTimeRollover)
{
    [[nodiscard]] static uint32_t uptimeAt(uint32_t ticks)
    {
        FreeRtosTaskFake_SetTickCount(ticks);
        return SolidSyslogFreeRtos_GetSysUpTime();
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosSysUpTimeRollover, AThousandTicksIsOneSecond)
{
    uint32_t before = uptimeAt(1000U);
    uint32_t after = uptimeAt(2000U);

    UNSIGNED_LONGS_EQUAL(100U, after - before);
}

TEST(SolidSyslogFreeRtosSysUpTimeRollover, KeepsCountingPastTheCounterWrap)
{
    uint32_t before = uptimeAt(UINT32_MAX);
    uint32_t after = uptimeAt(10000U);

    /* 10001 ticks on from UINT32_MAX, which is 1000 hundredths at 1000 Hz. */
    UNSIGNED_LONGS_EQUAL(1000U, after - before);
}
