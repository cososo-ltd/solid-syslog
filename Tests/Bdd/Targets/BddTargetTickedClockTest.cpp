#include "BddTargetTickedClock.h"
#include "CppUTest/TestHarness.h"
#include "SolidSyslogTimestamp.h"

#include <cstdint>

enum
{
    COUNTS_PER_MICROSECOND = 216,
    COUNTS_PER_SECOND = 216000000,

    /* 2026-11-14T18:40:57Z, a Saturday in a year that is not a leap year. */
    AN_INSTANT = 1794681657
};

#define CHECK_DATE(year, month, day)              \
    {                                             \
        const SolidSyslogTimestamp read = Read(); \
        LONGS_EQUAL((year), read.Year);           \
        LONGS_EQUAL((month), read.Month);         \
        LONGS_EQUAL((day), read.Day);             \
    }

#define CHECK_TIME(hour, minute, second, microsecond) \
    {                                                 \
        const SolidSyslogTimestamp read = Read();     \
        LONGS_EQUAL((hour), read.Hour);               \
        LONGS_EQUAL((minute), read.Minute);           \
        LONGS_EQUAL((second), read.Second);           \
        LONGS_EQUAL((microsecond), read.Microsecond); \
    }

static uint32_t counterNow;

/* Stands in for the interrupt: a tick that arrives while a reading is under
 * way, the given number of counter reads into the test. */
static int tickAtCounterRead;
static uint32_t tickSeconds;
static uint32_t tickCounter;
static int counterReads;

static uint32_t TestCounter(void)
{
    counterReads++;
    if (counterReads == tickAtCounterRead)
    {
        tickAtCounterRead = 0;
        counterNow = tickCounter;
        BddTargetTickedClock_Tick(tickSeconds);
    }
    return counterNow;
}

// clang-format off
TEST_GROUP(BddTargetTickedClock)
{
    void setup() override
    {
        counterNow = 0U;
        counterReads = 0;
        tickAtCounterRead = 0;
        BddTargetTickedClock_Initialise(TestCounter, COUNTS_PER_MICROSECOND);
    }

    static SolidSyslogTimestamp Read()
    {
        SolidSyslogTimestamp timestamp = {};
        timestamp.Year = 1; /* so a reading that fills nothing in is seen */
        BddTargetTickedClock_GetTimestamp(&timestamp);
        return timestamp;
    }

    static void TickAt(uint32_t seconds, uint32_t counter)
    {
        counterNow = counter;
        BddTargetTickedClock_Tick(seconds);
    }
};

// clang-format on

TEST(BddTargetTickedClock, HasNoTimeUntilItIsToldIt)
{
    CHECK_FALSE(BddTargetTickedClock_IsSet());
}

TEST(BddTargetTickedClock, ReadsAsNoTimestampUntilItIsToldTheTime)
{
    CHECK_DATE(0, 0, 0);
    CHECK_TIME(0, 0, 0, 0);
}

TEST(BddTargetTickedClock, HasTheTimeOnceASecondHasTicked)
{
    TickAt(AN_INSTANT, 0U);
    CHECK_TRUE(BddTargetTickedClock_IsSet());
}

TEST(BddTargetTickedClock, ForgetsTheTimeWhenInitialised)
{
    TickAt(AN_INSTANT, 0U);
    BddTargetTickedClock_Initialise(TestCounter, COUNTS_PER_MICROSECOND);
    CHECK_FALSE(BddTargetTickedClock_IsSet());
    CHECK_DATE(0, 0, 0);
}

TEST(BddTargetTickedClock, ReadsTheTimeOfDayOfTheSecondThatTicked)
{
    TickAt(AN_INSTANT, 0U);
    CHECK_TIME(18, 40, 57, 0);
}

TEST(BddTargetTickedClock, ReadsTheDateOfTheSecondThatTicked)
{
    TickAt(AN_INSTANT, 0U);
    CHECK_DATE(2026, 11, 14);
}

TEST(BddTargetTickedClock, ReadsInUtc)
{
    TickAt(AN_INSTANT, 0U);
    LONGS_EQUAL(0, Read().UtcOffsetMinutes);
}

TEST(BddTargetTickedClock, ReadsTheMicrosecondsTheCounterHasCountedSinceTheTick)
{
    TickAt(AN_INSTANT, 0U);
    counterNow = 250000U * COUNTS_PER_MICROSECOND;
    CHECK_TIME(18, 40, 57, 250000);
}

TEST(BddTargetTickedClock, CountsFromWhereTheCounterStoodAtTheTick)
{
    TickAt(AN_INSTANT, 5000U);
    counterNow = 5000U + (3U * COUNTS_PER_MICROSECOND);
    CHECK_TIME(18, 40, 57, 3);
}

TEST(BddTargetTickedClock, APartOfAMicrosecondIsNotCounted)
{
    TickAt(AN_INSTANT, 0U);
    counterNow = (2U * COUNTS_PER_MICROSECOND) - 1U;
    CHECK_TIME(18, 40, 57, 1);
}

TEST(BddTargetTickedClock, CountsThroughTheCounterWrapping)
{
    TickAt(AN_INSTANT, UINT32_MAX - 99U);
    counterNow = (7U * COUNTS_PER_MICROSECOND) - 100U;
    CHECK_TIME(18, 40, 57, 7);
}

TEST(BddTargetTickedClock, ReadsTheLastMicrosecondOfTheSecond)
{
    TickAt(AN_INSTANT, 0U);
    counterNow = COUNTS_PER_SECOND - 1U;
    CHECK_TIME(18, 40, 57, 999999);
}

/* The counter's oscillator runs a little fast against the hardware clock's. */
TEST(BddTargetTickedClock, StaysInItsSecondWhenTheCounterReachesASecondBeforeTheTick)
{
    TickAt(AN_INSTANT, 0U);
    counterNow = COUNTS_PER_SECOND;
    CHECK_TIME(18, 40, 57, 999999);
}

TEST(BddTargetTickedClock, StaysInItsSecondWhenTheTickIsSecondsLate)
{
    TickAt(AN_INSTANT, 0U);
    counterNow = 3U * COUNTS_PER_SECOND;
    CHECK_TIME(18, 40, 57, 999999);
}

TEST(BddTargetTickedClock, TheNextTickStartsTheNextSecond)
{
    TickAt(AN_INSTANT, 0U);
    TickAt(AN_INSTANT + 1U, COUNTS_PER_SECOND + 40U);
    CHECK_TIME(18, 40, 58, 0);
}

TEST(BddTargetTickedClock, SetPartWayThroughASecondReadsThatFarIntoIt)
{
    counterNow = 1000U;
    BddTargetTickedClock_Set(AN_INSTANT, 600000U);
    CHECK_TRUE(BddTargetTickedClock_IsSet());
    CHECK_TIME(18, 40, 57, 600000);
}

TEST(BddTargetTickedClock, SetPartWayThroughASecondCountsOnFromThere)
{
    counterNow = 1000U;
    BddTargetTickedClock_Set(AN_INSTANT, 600000U);
    counterNow = 1000U + (25U * COUNTS_PER_MICROSECOND);
    CHECK_TIME(18, 40, 57, 600025);
}

TEST(BddTargetTickedClock, ATargetThatWiresNoCounterReadsWholeSecondsRatherThanCrashing)
{
    BddTargetTickedClock_Initialise(nullptr, COUNTS_PER_MICROSECOND);
    BddTargetTickedClock_Tick(AN_INSTANT);
    CHECK_TIME(18, 40, 57, 0);
}

TEST(BddTargetTickedClock, ACounterSaidToCountNothingIsTakenToCountMicroseconds)
{
    BddTargetTickedClock_Initialise(TestCounter, 0U);
    TickAt(AN_INSTANT, 0U);
    counterNow = 42U;
    CHECK_TIME(18, 40, 57, 42);
}

TEST(BddTargetTickedClock, ATickDuringAReadingGivesTheTimeAfterTheTick)
{
    TickAt(AN_INSTANT, 0U);
    counterNow = COUNTS_PER_SECOND - 1U;
    counterReads = 0;
    tickAtCounterRead = 1;
    tickSeconds = AN_INSTANT + 1U;
    tickCounter = COUNTS_PER_SECOND;
    CHECK_TIME(18, 40, 58, 0);
}
