#include "BddTargetCalendar.h"
#include "CppUTest/TestHarness.h"
#include "SolidSyslogTimestamp.h"

#include <cstdint>

enum
{
    SECONDS_PER_DAY = 86400,

    /* A stride that is no multiple of a minute, an hour or a day, so the
     * round trip lands on every second of the minute and hour of the day. */
    ROUND_TRIP_STRIDE = 99991
};

#define CHECK_CALENDAR(seconds, year, month, day, hour, minute, second) \
    {                                                                   \
        const SolidSyslogTimestamp read = FromEpoch(seconds);           \
        LONGS_EQUAL((year), read.Year);                                 \
        LONGS_EQUAL((month), read.Month);                               \
        LONGS_EQUAL((day), read.Day);                                   \
        LONGS_EQUAL((hour), read.Hour);                                 \
        LONGS_EQUAL((minute), read.Minute);                             \
        LONGS_EQUAL((second), read.Second);                             \
    }

#define CHECK_EPOCH(seconds, year, month, day, hour, minute, second) \
    UNSIGNED_LONGS_EQUAL((seconds), ToEpoch((year), (month), (day), (hour), (minute), (second)))

// clang-format off
TEST_GROUP(BddTargetCalendar)
{
    static SolidSyslogTimestamp FromEpoch(uint32_t seconds)
    {
        SolidSyslogTimestamp timestamp = {};
        BddTargetCalendar_FromEpoch(seconds, &timestamp);
        return timestamp;
    }

    static uint32_t ToEpoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
    {
        SolidSyslogTimestamp timestamp = {};
        timestamp.Year = year;
        timestamp.Month = month;
        timestamp.Day = day;
        timestamp.Hour = hour;
        timestamp.Minute = minute;
        timestamp.Second = second;
        return BddTargetCalendar_ToEpoch(&timestamp);
    }
};

// clang-format on

TEST(BddTargetCalendar, TheEpochIsTheStartOf1970)
{
    CHECK_CALENDAR(0U, 1970, 1, 1, 0, 0, 0);
}

TEST(BddTargetCalendar, GivesTheTimeOfDay)
{
    CHECK_CALENDAR(1794681657U, 2026, 11, 14, 18, 40, 57);
}

TEST(BddTargetCalendar, LeavesTheMicrosecondsAndTheOffsetAtZero)
{
    SolidSyslogTimestamp timestamp = {};
    timestamp.Microsecond = 5U;
    timestamp.UtcOffsetMinutes = 60;
    BddTargetCalendar_FromEpoch(0U, &timestamp);
    LONGS_EQUAL(0, timestamp.Microsecond);
    LONGS_EQUAL(0, timestamp.UtcOffsetMinutes);
}

TEST(BddTargetCalendar, GivesTheLastSecondOfADay)
{
    CHECK_CALENDAR(86399U, 1970, 1, 1, 23, 59, 59);
}

TEST(BddTargetCalendar, GivesTheFirstSecondOfTheNextDay)
{
    CHECK_CALENDAR(86400U, 1970, 1, 2, 0, 0, 0);
}

TEST(BddTargetCalendar, GivesTheLeapDayOfALeapYear)
{
    CHECK_CALENDAR(1709164800U, 2024, 2, 29, 0, 0, 0);
}

TEST(BddTargetCalendar, GivesTheDayAfterALeapDay)
{
    CHECK_CALENDAR(1709251200U, 2024, 3, 1, 0, 0, 0);
}

/* 2000 is a leap year though it is divisible by 100, because it is by 400. */
TEST(BddTargetCalendar, GivesTheLeapDayOfACenturyThatHasOne)
{
    CHECK_CALENDAR(951782400U, 2000, 2, 29, 0, 0, 0);
}

TEST(BddTargetCalendar, GivesTheLastSecondOfAYear)
{
    CHECK_CALENDAR(1798761599U, 2026, 12, 31, 23, 59, 59);
}

/* 2100 is not a leap year, and the last second thirty-two bits hold is in 2106. */
TEST(BddTargetCalendar, GivesTheLastSecondThirtyTwoBitsHold)
{
    CHECK_CALENDAR(UINT32_MAX, 2106, 2, 7, 6, 28, 15);
}

TEST(BddTargetCalendar, TheStartOf1970IsTheEpoch)
{
    CHECK_EPOCH(0U, 1970, 1, 1, 0, 0, 0);
}

TEST(BddTargetCalendar, CountsTheTimeOfDay)
{
    CHECK_EPOCH(1794681657U, 2026, 11, 14, 18, 40, 57);
}

TEST(BddTargetCalendar, CountsALeapDay)
{
    CHECK_EPOCH(1709164800U, 2024, 2, 29, 0, 0, 0);
}

TEST(BddTargetCalendar, CountsTheDayAfterALeapDay)
{
    CHECK_EPOCH(1709251200U, 2024, 3, 1, 0, 0, 0);
}

TEST(BddTargetCalendar, CountsTheLeapDayOfACenturyThatHasOne)
{
    CHECK_EPOCH(951782400U, 2000, 2, 29, 0, 0, 0);
}

TEST(BddTargetCalendar, CountsTheLastSecondOfAYear)
{
    CHECK_EPOCH(1798761599U, 2026, 12, 31, 23, 59, 59);
}

TEST(BddTargetCalendar, CountsTheLastSecondThirtyTwoBitsHold)
{
    CHECK_EPOCH(UINT32_MAX, 2106, 2, 7, 6, 28, 15);
}

TEST(BddTargetCalendar, TakesTheDateAsUtcWhateverItsOffset)
{
    SolidSyslogTimestamp timestamp = {};
    timestamp.Year = 1970;
    timestamp.Month = 1;
    timestamp.Day = 1;
    timestamp.UtcOffsetMinutes = 60;
    UNSIGNED_LONGS_EQUAL(0U, BddTargetCalendar_ToEpoch(&timestamp));
}

TEST(BddTargetCalendar, EverySecondSampledComesBackFromItsDateAndTime)
{
    uint32_t seconds = 0U;
    bool more = true;
    while (more)
    {
        SolidSyslogTimestamp timestamp = FromEpoch(seconds);
        UNSIGNED_LONGS_EQUAL(seconds, BddTargetCalendar_ToEpoch(&timestamp));
        more = seconds <= (UINT32_MAX - ROUND_TRIP_STRIDE);
        seconds += ROUND_TRIP_STRIDE;
    }
}
