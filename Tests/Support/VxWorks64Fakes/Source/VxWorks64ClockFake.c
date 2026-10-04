#include "vxWorks.h"

#include "VxWorks64ClockFake.h"

#include <stdbool.h>
#include <time.h>

static bool VxWorks64ClockFake_ClockGettimeFails = false;

void VxWorks64ClockFake_Reset(void)
{
    VxWorks64ClockFake_ClockGettimeFails = false;
}

void VxWorks64ClockFake_FailClockGettime(void)
{
    VxWorks64ClockFake_ClockGettimeFails = true;
}

int clock_gettime(clockid_t clockId, struct timespec* tp)
{
    (void) clockId;
    (void) tp;
    return VxWorks64ClockFake_ClockGettimeFails ? ERROR : OK;
}
