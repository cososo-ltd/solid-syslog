/* ST's NUCLEO-F767ZI behind the board seam's clock - see BddTargetBoardClock.h.
 *
 * Not wired to the part's real-time clock yet, so the board has no clock to
 * set and none to read. */

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
