/* The QEMU mps2-an385 machine's clock - see BddTargetBoardClock.h.
 *
 * The machine is run as a product with no time-of-day clock, so there is none
 * to set or read. */

#include "BddTargetBoardClock.h"

#include <string.h>

bool BddTargetBoardClock_Set(uint32_t secondsSinceEpoch)
{
    (void) secondsSinceEpoch;
    return false;
}

bool BddTargetBoardClock_IsSet(void)
{
    return false;
}

void BddTargetBoardClock_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    memset(timestamp, 0, sizeof(*timestamp));
}
