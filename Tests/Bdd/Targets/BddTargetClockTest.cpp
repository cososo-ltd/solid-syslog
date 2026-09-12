#include "BddTargetClock.h"
#include "CppUTest/TestHarness.h"

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
    LONGS_EQUAL(BDD_TARGET_BUILD_EPOCH, BddTargetClock_Now());
}

TEST(BddTargetClock, ATargetThatWiresNoUptimeStillReadsTheEpochRatherThanCrashing)
{
    BddTargetClock_Initialise(nullptr);
    LONGS_EQUAL(BDD_TARGET_BUILD_EPOCH, BddTargetClock_Now());
}

TEST(BddTargetClock, AdvancesWithTheTimeTheDeviceHasBeenRunning)
{
    uptimeSeconds = 90U;
    LONGS_EQUAL(BDD_TARGET_BUILD_EPOCH + 90, BddTargetClock_Now());
}
