/* The MPS2-AN385 behind the board seam's clock - see BddTargetBoardClock.h.
 *
 * The machine is run as a product with no clock of the time of day, so there
 * is none to set and none to read. */

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
