#include "ClockFake.h"

#include <string.h>
#include <sys/types.h>

struct timespec;
struct tm;

static time_t fakeSeconds;
static long   fakeNanoseconds;
static int    clockGettimeReturn;
static int    gmtimeFailure;

void ClockFake_Reset(void)
{
    fakeSeconds        = 0;
    fakeNanoseconds    = 0;
    clockGettimeReturn = 0;
    gmtimeFailure      = 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- mirrors POSIX timespec (seconds + nanoseconds)
void ClockFake_SetTime(time_t seconds, long nanoseconds)
{
    fakeSeconds     = seconds;
    fakeNanoseconds = nanoseconds;
}

void ClockFake_SetClockGettimeReturn(int returnValue)
{
    clockGettimeReturn = returnValue;
}

// cppcheck-suppress constParameter -- API allows NULL to signal failure; const would be misleading
void ClockFake_SetGmtimeReturn(struct tm* returnValue)
{
    gmtimeFailure = (returnValue == NULL) ? 1 : 0;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name) -- overriding system function with matching semantics
int clock_gettime(clockid_t clockId, struct timespec* tp)
{
    (void) clockId;
    if (clockGettimeReturn == 0)
    {
        tp->tv_sec  = fakeSeconds;
        tp->tv_nsec = fakeNanoseconds;
    }
    return clockGettimeReturn;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name) -- overriding system function with matching semantics
struct tm* gmtime_r(const time_t* timep, struct tm* result)
{
    if (gmtimeFailure)
    {
        return NULL;
    }

    const struct tm* breakdown = gmtime(timep);
    if (breakdown == NULL)
    {
        return NULL;
    }

    memcpy(result, breakdown, sizeof(struct tm));
    return result;
}
