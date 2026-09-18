#include "BddTargetClock.h"

#include <stddef.h>

static uint32_t Clock_NoUptime(void);

static BddTargetClockUptimeFunction clockUptime = Clock_NoUptime;

void BddTargetClock_Initialise(BddTargetClockUptimeFunction uptime)
{
    clockUptime = (uptime != NULL) ? uptime : Clock_NoUptime;
}

int64_t BddTargetClock_Now(void)
{
    return (int64_t) BDD_TARGET_BUILD_EPOCH + (int64_t) clockUptime();
}

/* A target that wires no uptime still has a clock, stopped at the instant the
   image was built. That is enough for certificate validity, which asks only
   which side of a window the device is on. */
static uint32_t Clock_NoUptime(void)
{
    return 0U;
}
