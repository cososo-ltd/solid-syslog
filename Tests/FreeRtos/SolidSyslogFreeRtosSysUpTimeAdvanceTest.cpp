/* The tick counter cannot say how many times it has rolled over, so carrying
 * uptime past its rollover needs state of its own. Advance holds that state
 * and the scaling together as a pure function, so the rollover can be driven
 * at a tick rate that does not divide 100 - the 1000 Hz FreeRTOS default
 * among them - without the test build having to be compiled at that rate.
 *
 * RFC 3418 TimeTicks is hundredths of a second in a uint32, so the reported
 * value is meant to wrap at 2^32 hundredths - about 497 days - and nowhere
 * earlier. See #755. */
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFreeRtosSysUpTimePrivate.h"

namespace
{
constexpr uint32_t HZ_1000 = 1000U;
constexpr uint32_t HZ_100 = 100U;
constexpr uint64_t TICK_ROLLOVER = 0x100000000ULL;
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosSysUpTimeAdvance)
{
    struct SolidSyslogFreeRtosSysUpTimeState state = {};

    /** What the public entry point does: extend under the state, then scale. */
    [[nodiscard]] uint32_t advance(uint64_t ticks, uint32_t rateHz)
    {
        return SolidSyslogFreeRtosSysUpTime_Hundredths(
            SolidSyslogFreeRtosSysUpTime_Extend(&state, ticks), rateHz
        );
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosSysUpTimeAdvance, ScalesTicksToHundredthsAtATickRateThatDoesNotDivide100)
{
    UNSIGNED_LONGS_EQUAL(100U, advance(1000U, HZ_1000));
}

TEST(SolidSyslogFreeRtosSysUpTimeAdvance, KeepsCountingPastTheTickCounterRollover)
{
    (void) advance(UINT32_MAX, HZ_1000);

    /* One tick later the counter has wrapped to zero. Uptime has not. */
    UNSIGNED_LONGS_EQUAL((uint32_t) (TICK_ROLLOVER / 10U), advance(0U, HZ_1000));
}

TEST(SolidSyslogFreeRtosSysUpTimeAdvance, IsMonotonicAcrossSuccessiveRollovers)
{
    uint32_t previous = 0U;
    for (int rollover = 0; rollover < 4; ++rollover)
    {
        (void) advance(UINT32_MAX, HZ_1000);
        uint32_t next = advance(0U, HZ_1000);
        CHECK(next > previous);
        previous = next;
    }
}

TEST(SolidSyslogFreeRtosSysUpTimeAdvance, WrapsOnlyWhenTheHundredthsThemselvesWrap)
{
    /* 2^32 hundredths at 1000 Hz is ten tick rollovers away. */
    for (int rollover = 0; rollover < 10; ++rollover)
    {
        (void) advance(UINT32_MAX, HZ_1000);
        (void) advance(0U, HZ_1000);
    }

    UNSIGNED_LONGS_EQUAL(0U, advance(0U, HZ_1000));
}

TEST(SolidSyslogFreeRtosSysUpTimeAdvance, AtOneHundredHertzATickIsAHundredth)
{
    UNSIGNED_LONGS_EQUAL(12345U, advance(12345U, HZ_100));
}

/* A 64-bit counter does not wrap within the life of the device, which it
 * reports as no modulus. The tick value must reach the scaling whole rather
 * than truncated to its low word. */
TEST(SolidSyslogFreeRtosSysUpTimeAdvance, CarriesATickCountWiderThanThirtyTwoBits)
{
    UNSIGNED_LONGS_EQUAL(
        SolidSyslogFreeRtosSysUpTime_Hundredths(TICK_ROLLOVER + 5000U, HZ_1000),
        advance((uint32_t) 0U + TICK_ROLLOVER + 5000U, HZ_1000)
    );
}
