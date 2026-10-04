#include "vxWorks.h"

#include "VxWorks64ClockFake.h"

#include <stdbool.h>
#include <time.h>

static bool VxWorks64ClockFake_ClockGettimeFails = false;
static bool VxWorks64ClockFake_GmtimeRFails = false;
static struct tm VxWorks64ClockFake_BrokenDown;
static long VxWorks64ClockFake_Nanoseconds = 0L;

void VxWorks64ClockFake_Reset(void)
{
    VxWorks64ClockFake_ClockGettimeFails = false;
    VxWorks64ClockFake_GmtimeRFails = false;
    VxWorks64ClockFake_BrokenDown = (struct tm) {0};
    VxWorks64ClockFake_Nanoseconds = 0L;
}

void VxWorks64ClockFake_FailClockGettime(void)
{
    VxWorks64ClockFake_ClockGettimeFails = true;
}

void VxWorks64ClockFake_FailGmtimeR(void)
{
    VxWorks64ClockFake_GmtimeRFails = true;
}

void VxWorks64ClockFake_SetBrokenDownTime(int year, int month, int day, int hour, int minute, int second)
{
    VxWorks64ClockFake_BrokenDown.tm_year = year;
    VxWorks64ClockFake_BrokenDown.tm_mon = month;
    VxWorks64ClockFake_BrokenDown.tm_mday = day;
    VxWorks64ClockFake_BrokenDown.tm_hour = hour;
    VxWorks64ClockFake_BrokenDown.tm_min = minute;
    VxWorks64ClockFake_BrokenDown.tm_sec = second;
}

void VxWorks64ClockFake_SetNanoseconds(long nanoseconds)
{
    VxWorks64ClockFake_Nanoseconds = nanoseconds;
}

int clock_gettime(clockid_t clockId, struct timespec* tp)
{
    (void) clockId;
    tp->tv_sec = 0UL;
    tp->tv_nsec = VxWorks64ClockFake_Nanoseconds;
    return VxWorks64ClockFake_ClockGettimeFails ? ERROR : OK;
}

int gmtime_r(const time_t* tod, struct tm* result)
{
    (void) tod;
    *result = VxWorks64ClockFake_BrokenDown;
    return VxWorks64ClockFake_GmtimeRFails ? ERROR : OK;
}
