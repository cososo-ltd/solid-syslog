#include "BddTargetClock.h"
#include "CppUTest/TestHarness.h"

#define CHECK_CLOCK_NOW(expected) LONGS_EQUAL((expected), BddTargetClock_Now())

static uint32_t uptimeSeconds;

static uint32_t TestUptime(void)
{
    return uptimeSeconds;
}

// clang-format off
TEST_GROUP(BddTargetClock)
{
    void setup() override
    {
        uptimeSeconds = 0U;
        BddTargetClock_Initialise(TestUptime);
    }
};

// clang-format on

TEST(BddTargetClock, StartsAtTheEpochTheBuildWasSeededWith)
{
    CHECK_CLOCK_NOW(BDD_TARGET_BUILD_EPOCH);
}

TEST(BddTargetClock, ATargetThatWiresNoUptimeStillReadsTheEpochRatherThanCrashing)
{
    BddTargetClock_Initialise(nullptr);
    CHECK_CLOCK_NOW(BDD_TARGET_BUILD_EPOCH);
}

TEST(BddTargetClock, AdvancesWithTheTimeTheDeviceHasBeenRunning)
{
    uptimeSeconds = 90U;
    CHECK_CLOCK_NOW(BDD_TARGET_BUILD_EPOCH + 90);
}
