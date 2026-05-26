#include <time.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogPosixClock.h"
#include "SolidSyslogTimestamp.h"
#include "ClockFake.h"

// 2025-04-02T00:00:00Z
static const time_t TEST_EPOCH = 1743552000;

// clang-format off
#define GET_TIMESTAMP()              getTimestamp()
#define CHECK_YEAR(expected)         LONGS_EQUAL(expected, GET_TIMESTAMP().Year)
#define CHECK_MONTH(expected)        LONGS_EQUAL(expected, GET_TIMESTAMP().Month)
#define CHECK_DAY(expected)          LONGS_EQUAL(expected, GET_TIMESTAMP().Day)
#define CHECK_HOUR(expected)         LONGS_EQUAL(expected, GET_TIMESTAMP().Hour)
#define CHECK_MINUTE(expected)       LONGS_EQUAL(expected, GET_TIMESTAMP().Minute)
#define CHECK_SECOND(expected)       LONGS_EQUAL(expected, GET_TIMESTAMP().Second)
#define CHECK_MICROSECOND(expected)  LONGS_EQUAL(expected, GET_TIMESTAMP().Microsecond)
#define CHECK_UTC_OFFSET(expected)   LONGS_EQUAL(expected, GET_TIMESTAMP().UtcOffsetMinutes)
#define CHECK_MONTH_IS_INVALID()     LONGS_EQUAL(0, GET_TIMESTAMP().Month)
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogPosixClock)
{
    void setup() override
    {
        ClockFake_Reset();
        ClockFake_SetTime(TEST_EPOCH, 0);
    }

    static struct SolidSyslogTimestamp getTimestamp()
    {
        struct SolidSyslogTimestamp ts = {};
        SolidSyslogPosixClock_GetTimestamp(&ts);
        return ts;
    }
};

// clang-format on

TEST(SolidSyslogPosixClock, YearMatchesKnownTime)
{
    CHECK_YEAR(2025);
}

TEST(SolidSyslogPosixClock, MonthMatchesKnownTime)
{
    CHECK_MONTH(4);
}

TEST(SolidSyslogPosixClock, DayMatchesKnownTime)
{
    CHECK_DAY(2);
}

TEST(SolidSyslogPosixClock, HourMatchesKnownTime)
{
    CHECK_HOUR(0);
}

TEST(SolidSyslogPosixClock, MinuteMatchesKnownTime)
{
    CHECK_MINUTE(0);
}

TEST(SolidSyslogPosixClock, SecondMatchesKnownTime)
{
    CHECK_SECOND(0);
}

TEST(SolidSyslogPosixClock, MicrosecondFromNanoseconds)
{
    ClockFake_SetTime(TEST_EPOCH, 123456789);
    CHECK_MICROSECOND(123456);
}

TEST(SolidSyslogPosixClock, UtcOffsetIsAlwaysZero)
{
    CHECK_UTC_OFFSET(0);
}

TEST(SolidSyslogPosixClock, ClockGettimeFailureReturnsInvalidTimestamp)
{
    ClockFake_SetClockGettimeReturn(-1);
    CHECK_MONTH_IS_INVALID();
}

TEST(SolidSyslogPosixClock, GmtimeFailureReturnsInvalidTimestamp)
{
    ClockFake_SetGmtimeReturn(nullptr);
    CHECK_MONTH_IS_INVALID();
}

// 2025-01-01T00:00:00Z
TEST(SolidSyslogPosixClock, Month1FromJanuaryEpoch)
{
    ClockFake_SetTime(1735689600, 0);
    CHECK_MONTH(1);
}

// 2025-12-31T00:00:00Z
TEST(SolidSyslogPosixClock, Month12Day31FromDecemberEpoch)
{
    ClockFake_SetTime(1767139200, 0);
    CHECK_MONTH(12);
    CHECK_DAY(31);
}

// 2025-04-01T00:00:00Z
TEST(SolidSyslogPosixClock, Day1FromFirstOfMonth)
{
    ClockFake_SetTime(1743465600, 0);
    CHECK_DAY(1);
}

// 2025-04-02T23:59:59Z
TEST(SolidSyslogPosixClock, Hour23Minute59Second59)
{
    ClockFake_SetTime(1743638399, 0);
    CHECK_HOUR(23);
    CHECK_MINUTE(59);
    CHECK_SECOND(59);
}

TEST(SolidSyslogPosixClock, MaxNanosecondsProducesMaxMicroseconds)
{
    ClockFake_SetTime(TEST_EPOCH, 999999999);
    CHECK_MICROSECOND(999999);
}

TEST(SolidSyslogPosixClock, ZeroNanosecondsProducesZeroMicroseconds)
{
    ClockFake_SetTime(TEST_EPOCH, 0);
    CHECK_MICROSECOND(0);
}

// 1970-01-01T00:00:00Z
TEST(SolidSyslogPosixClock, EpochZeroProduces1970)
{
    ClockFake_SetTime(0, 0);
    CHECK_YEAR(1970);
    CHECK_MONTH(1);
    CHECK_DAY(1);
    CHECK_HOUR(0);
    CHECK_MINUTE(0);
    CHECK_SECOND(0);
}

// 2038-01-19T03:14:07Z
TEST(SolidSyslogPosixClock, Max32BitEpochProduces2038)
{
    ClockFake_SetTime(2147483647, 0);
    CHECK_YEAR(2038);
    CHECK_MONTH(1);
    CHECK_DAY(19);
    CHECK_HOUR(3);
    CHECK_MINUTE(14);
    CHECK_SECOND(7);
}

// 2066-01-01T00:00:00Z — well beyond 32-bit time_t limit
TEST(SolidSyslogPosixClock, NoY2038LimitOnThisPlatform)
{
    ClockFake_SetTime(3029529600, 0);
    CHECK_YEAR(2066);
    CHECK_MONTH(1);
    CHECK_DAY(1);
}

TEST(SolidSyslogPosixClock, AllFieldsInValidRanges)
{
    struct SolidSyslogTimestamp ts = getTimestamp();
    CHECK(ts.Year > 0);
    CHECK((ts.Month >= 1) && (ts.Month <= 12));
    CHECK((ts.Day >= 1) && (ts.Day <= 31));
    CHECK(ts.Hour <= 23);
    CHECK(ts.Minute <= 59);
    CHECK(ts.Second <= 59);
    CHECK(ts.Microsecond <= 999999);
    LONGS_EQUAL(0, ts.UtcOffsetMinutes);
}
