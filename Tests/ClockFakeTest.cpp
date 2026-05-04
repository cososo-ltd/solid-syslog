#include <time.h>

#include "ClockFake.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(ClockFake)
{
    void setup() override
    {
        ClockFake_Reset();
    }
};

// clang-format on

TEST(ClockFake, ClockGettimeReturnsZeroAfterReset)
{
    struct timespec ts = {0, 0};
    LONGS_EQUAL(0, clock_gettime(CLOCK_REALTIME, &ts));
}

TEST(ClockFake, ClockGettimeReturnsConfiguredSeconds)
{
    ClockFake_SetTime(12345, 0);
    struct timespec ts = {0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);
    LONGS_EQUAL(12345, ts.tv_sec);
}

TEST(ClockFake, ClockGettimeReturnsConfiguredNanoseconds)
{
    ClockFake_SetTime(0, 999000);
    struct timespec ts = {0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);
    LONGS_EQUAL(999000, ts.tv_nsec);
}

TEST(ClockFake, ClockGettimeFailureReturnsNonZero)
{
    ClockFake_SetClockGettimeReturn(-1);
    struct timespec ts = {0, 0};
    LONGS_EQUAL(-1, clock_gettime(CLOCK_REALTIME, &ts));
}

TEST(ClockFake, ClockGettimeFailureDoesNotModifyTimespec)
{
    ClockFake_SetTime(12345, 67890);
    ClockFake_SetClockGettimeReturn(-1);
    struct timespec ts = {0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);
    LONGS_EQUAL(0, ts.tv_sec);
    LONGS_EQUAL(0, ts.tv_nsec);
}

TEST(ClockFake, GmtimeReturnsNonNullAfterReset)
{
    time_t    seconds = 0;
    struct tm result  = {};
    CHECK(gmtime_r(&seconds, &result) != nullptr);
}

TEST(ClockFake, GmtimePopulatesResult)
{
    time_t    seconds = 1743552000;
    struct tm result  = {};
    gmtime_r(&seconds, &result);
    LONGS_EQUAL(2025 - 1900, result.tm_year);
}

TEST(ClockFake, GmtimeFailureReturnsNull)
{
    ClockFake_SetGmtimeReturn(nullptr);
    time_t    seconds = 0;
    struct tm result  = {};
    POINTERS_EQUAL(nullptr, gmtime_r(&seconds, &result));
}
