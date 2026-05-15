#include "CppUTest/TestHarness.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogWindowsClock.h"
#include "SolidSyslogWindowsClockInternal.h"

#include <windows.h>

// 2025-04-02T00:00:00Z — matches the POSIX clock test default so timestamps
// behave identically across platforms when the same wall-clock value is faked.
static SYSTEMTIME fakeSystemTime;
static FILETIME fakeFileTime;
static bool useRawFileTime;

static void WINAPI FakeGetSystemTimeAsFileTime(LPFILETIME fileTime)
{
    if (useRawFileTime)
    {
        *fileTime = fakeFileTime;
        return;
    }
    SystemTimeToFileTime(&fakeSystemTime, fileTime);
}

// clang-format off
// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- macros preserve __FILE__/__LINE__ in test failure output
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
// NOLINTEND(cppcoreguidelines-macro-usage)
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogWindowsClock)
{
    void setup() override
    {
        useRawFileTime = false;
        fakeSystemTime = SYSTEMTIME{};
        fakeSystemTime.wYear   = 2025;
        fakeSystemTime.wMonth  = 4;
        fakeSystemTime.wDay    = 2;
        fakeSystemTime.wHour   = 0;
        fakeSystemTime.wMinute = 0;
        fakeSystemTime.wSecond = 0;
        UT_PTR_SET(WindowsClock_GetSystemTimeAsFileTime, FakeGetSystemTimeAsFileTime);
    }

    static struct SolidSyslogTimestamp getTimestamp()
    {
        struct SolidSyslogTimestamp ts = {};
        SolidSyslogWindowsClock_GetTimestamp(&ts);
        return ts;
    }

    static void setFileTimeFromSystemTime(WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second)
    {
        useRawFileTime               = false;
        fakeSystemTime.wYear         = year;
        fakeSystemTime.wMonth        = month;
        fakeSystemTime.wDay          = day;
        fakeSystemTime.wHour         = hour;
        fakeSystemTime.wMinute       = minute;
        fakeSystemTime.wSecond       = second;
        fakeSystemTime.wMilliseconds = 0;
    }

    static void setRawFileTime(DWORD low, DWORD high)
    {
        useRawFileTime           = true;
        fakeFileTime.dwLowDateTime  = low;
        fakeFileTime.dwHighDateTime = high;
    }
};
// clang-format on

TEST(SolidSyslogWindowsClock, YearMatchesKnownTime)
{
    CHECK_YEAR(2025);
}

TEST(SolidSyslogWindowsClock, MonthMatchesKnownTime)
{
    CHECK_MONTH(4);
}

TEST(SolidSyslogWindowsClock, DayMatchesKnownTime)
{
    CHECK_DAY(2);
}

TEST(SolidSyslogWindowsClock, HourMatchesKnownTime)
{
    CHECK_HOUR(0);
}

TEST(SolidSyslogWindowsClock, MinuteMatchesKnownTime)
{
    CHECK_MINUTE(0);
}

TEST(SolidSyslogWindowsClock, SecondMatchesKnownTime)
{
    CHECK_SECOND(0);
}

TEST(SolidSyslogWindowsClock, UtcOffsetIsAlwaysZero)
{
    CHECK_UTC_OFFSET(0);
}

TEST(SolidSyslogWindowsClock, ZeroMicrosecondsProducesZeroMicroseconds)
{
    CHECK_MICROSECOND(0);
}

// FILETIME granularity is 100-ns; microsecond = (filetime % 10_000_000) / 10
TEST(SolidSyslogWindowsClock, MicrosecondFromHundredNanosecondRemainder)
{
    // One second after FILETIME epoch (1601-01-01) + 123_456 microseconds.
    // In 100-ns units: 10_000_000 + 1_234_560 = 11_234_560.
    setRawFileTime(11234560, 0);
    CHECK_MICROSECOND(123456);
}

TEST(SolidSyslogWindowsClock, MaxMicrosecondsFromRemainder)
{
    // 9_999_990 100-ns units → 999_999 microseconds.
    setRawFileTime(9999990, 0);
    CHECK_MICROSECOND(999999);
}

TEST(SolidSyslogWindowsClock, FileTimeToSystemTimeFailureReturnsInvalidTimestamp)
{
    // dwHighDateTime > 0x7FFFFFFF causes FileTimeToSystemTime to fail.
    setRawFileTime(0, 0x80000000);
    CHECK_MONTH_IS_INVALID();
}

TEST(SolidSyslogWindowsClock, Month1FromJanuary)
{
    setFileTimeFromSystemTime(2025, 1, 1, 0, 0, 0);
    CHECK_MONTH(1);
}

TEST(SolidSyslogWindowsClock, Month12Day31FromDecember)
{
    setFileTimeFromSystemTime(2025, 12, 31, 0, 0, 0);
    CHECK_MONTH(12);
    CHECK_DAY(31);
}

TEST(SolidSyslogWindowsClock, Hour23Minute59Second59)
{
    setFileTimeFromSystemTime(2025, 4, 2, 23, 59, 59);
    CHECK_HOUR(23);
    CHECK_MINUTE(59);
    CHECK_SECOND(59);
}

// FILETIME epoch is 1601-01-01 — all zeroes map to year 1601.
TEST(SolidSyslogWindowsClock, ZeroFileTimeProduces1601)
{
    setRawFileTime(0, 0);
    CHECK_YEAR(1601);
    CHECK_MONTH(1);
    CHECK_DAY(1);
}

TEST(SolidSyslogWindowsClock, AllFieldsInValidRanges)
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
