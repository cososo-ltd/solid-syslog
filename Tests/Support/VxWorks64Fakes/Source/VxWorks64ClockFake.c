#include "vxWorks.h"

#include "VxWorks64ClockFake.h"

#include <stdbool.h>
#include <time.h>

static bool VxWorks64ClockFake_ClockGettimeFails = false;
static bool VxWorks64ClockFake_GmtimeRFails = false;
static struct tm VxWorks64ClockFake_BrokenDown;
static long VxWorks64ClockFake_Nanoseconds = 0L;
static time_t VxWorks64ClockFake_Seconds = 0UL;
static clockid_t VxWorks64ClockFake_LastClockId = -1;
static time_t VxWorks64ClockFake_BrokenDownSeconds = 0UL;

void VxWorks64ClockFake_Reset(void)
{
    VxWorks64ClockFake_ClockGettimeFails = false;
    VxWorks64ClockFake_GmtimeRFails = false;
    VxWorks64ClockFake_BrokenDown = (struct tm) {0};
    VxWorks64ClockFake_Nanoseconds = 0L;
    VxWorks64ClockFake_Seconds = 0UL;
    VxWorks64ClockFake_LastClockId = -1;
    VxWorks64ClockFake_BrokenDownSeconds = 0UL;
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

void VxWorks64ClockFake_SetSeconds(unsigned long seconds)
{
    VxWorks64ClockFake_Seconds = seconds;
}

bool VxWorks64ClockFake_LastReadWasRealTime(void)
{
    return VxWorks64ClockFake_LastClockId == CLOCK_REALTIME;
}

unsigned long VxWorks64ClockFake_LastBrokenDownSeconds(void)
{
    return VxWorks64ClockFake_BrokenDownSeconds;
}

int clock_gettime(clockid_t clockId, struct timespec* tp)
{
    VxWorks64ClockFake_LastClockId = clockId;
    tp->tv_sec = VxWorks64ClockFake_Seconds;
    tp->tv_nsec = VxWorks64ClockFake_Nanoseconds;
    return VxWorks64ClockFake_ClockGettimeFails ? ERROR : OK;
}

int gmtime_r(const time_t* tod, struct tm* result)
{
    VxWorks64ClockFake_BrokenDownSeconds = *tod;
    *result = VxWorks64ClockFake_BrokenDown;
    return VxWorks64ClockFake_GmtimeRFails ? ERROR : OK;
}
