#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cstring>

#include "SolidSyslogTimestamp.h"
#include "SolidSyslogVxWorks64Clock.h"
#include "VxWorks64ClockFake.h"

// Asserts the clock signalled "no usable timestamp" by zeroing every field.
#define CHECK_TIMESTAMP_ZEROED(timestamp)                       \
    {                                                           \
        const struct SolidSyslogTimestamp zeroed = {};          \
        MEMCMP_EQUAL(&zeroed, &(timestamp), sizeof(timestamp)); \
    }

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Clock)
{
    struct SolidSyslogTimestamp timestamp = {};

    void setup() override
    {
        VxWorks64ClockFake_Reset();
        // 2026-10-04T13:45:30, as gmtime_r's raw fields.
        VxWorks64ClockFake_SetBrokenDownTime(126, 9, 4, 13, 45, 30);
        (void) memset(&timestamp, 0xA5, sizeof(timestamp));
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Clock, TimestampIsZeroedWhenTheClockFails)
{
    VxWorks64ClockFake_FailClockGettime();

    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    CHECK_TIMESTAMP_ZEROED(timestamp);
}

TEST(SolidSyslogVxWorks64Clock, YearCountsFromNineteenHundred)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(2026, timestamp.Year);
}

TEST(SolidSyslogVxWorks64Clock, MonthCountsFromOne)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(10, timestamp.Month);
}

TEST(SolidSyslogVxWorks64Clock, DayIsTheDayOfTheMonth)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(4, timestamp.Day);
}

TEST(SolidSyslogVxWorks64Clock, HourIsTheHourOfTheDay)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(13, timestamp.Hour);
}

TEST(SolidSyslogVxWorks64Clock, MinuteIsTheMinuteOfTheHour)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(45, timestamp.Minute);
}

TEST(SolidSyslogVxWorks64Clock, SecondIsTheSecondOfTheMinute)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(30, timestamp.Second);
}

TEST(SolidSyslogVxWorks64Clock, MicrosecondTruncatesTheNanoseconds)
{
    VxWorks64ClockFake_SetNanoseconds(123456789L);

    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(123456, timestamp.Microsecond);
}

TEST(SolidSyslogVxWorks64Clock, TimestampIsZeroedWhenTheBreakdownFails)
{
    VxWorks64ClockFake_FailGmtimeR();

    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    CHECK_TIMESTAMP_ZEROED(timestamp);
}

TEST(SolidSyslogVxWorks64Clock, ReadsTheRealTimeClock)
{
    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    CHECK_TRUE(VxWorks64ClockFake_LastReadWasRealTime());
}

TEST(SolidSyslogVxWorks64Clock, BreaksDownTheSecondsTheClockRead)
{
    VxWorks64ClockFake_SetSeconds(1791119130UL);

    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    UNSIGNED_LONGS_EQUAL(1791119130UL, VxWorks64ClockFake_LastBrokenDownSeconds());
}
