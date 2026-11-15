#ifndef BDD_TARGET_BOARD_CLOCK_H
#define BDD_TARGET_BOARD_CLOCK_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimestamp.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The board's clock of the time of day. Part of the board seam - see
     * BddTargetBoard.h - and defined in the board's directory.
     *
     * A board with no such clock is the product RFC 5424 section 6.2.3.1
     * describes, one that cannot obtain the time: it refuses to be told it,
     * its timestamp is always the zeroed one the library sends as NILVALUE,
     * and it is never set. A board with one behaves the same until it knows
     * the time, whether it is told it now or finds its clock still running
     * from before a reset. */

    /* Tells the board the time, in seconds since the Unix epoch, in UTC.
     * False from a board with no clock to tell, or whose clock did not take
     * it. Called from a task, with the scheduler running. */
    bool BddTargetBoardClock_Set(uint32_t secondsSinceEpoch);

    /* True once the board knows the time. */
    bool BddTargetBoardClock_IsSet(void);

    /* A SolidSyslogClockFunction, so safe to call from any task at any time:
     * the time in UTC, or the timestamp zeroed while the board does not know
     * it. */
    void BddTargetBoardClock_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_CLOCK_H */
