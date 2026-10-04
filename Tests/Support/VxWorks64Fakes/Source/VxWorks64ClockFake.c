#include "vxWorks.h"

#include "VxWorks64ClockFake.h"

#include <stdbool.h>
#include <time.h>

static bool VxWorks64ClockFake_ClockGettimeFails = false;
static struct tm VxWorks64ClockFake_BrokenDown;

void VxWorks64ClockFake_Reset(void)
{
    VxWorks64ClockFake_ClockGettimeFails = false;
    VxWorks64ClockFake_BrokenDown = (struct tm) {0};
}

void VxWorks64ClockFake_FailClockGettime(void)
{
    VxWorks64ClockFake_ClockGettimeFails = true;
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

int clock_gettime(clockid_t clockId, struct timespec* tp)
{
    (void) clockId;
    (void) tp;
    return VxWorks64ClockFake_ClockGettimeFails ? ERROR : OK;
}

int gmtime_r(const time_t* tod, struct tm* result)
{
    (void) tod;
    *result = VxWorks64ClockFake_BrokenDown;
    return OK;
}
