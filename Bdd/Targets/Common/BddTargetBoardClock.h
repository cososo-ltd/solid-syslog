#ifndef BDD_TARGET_BOARD_CLOCK_H
#define BDD_TARGET_BOARD_CLOCK_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimestamp.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The board's time-of-day clock. Part of the board seam - see
     * BddTargetBoard.h - and defined in the board's directory.
     *
     * A board with no such clock cannot obtain the time (RFC 5424 section
     * 6.2.3.1): Set returns false, IsSet stays false, and the timestamp is
     * always zeroed, which the library sends as NILVALUE. A board with one
     * behaves the same until it is told the time, or finds its clock still
     * running from before a reset. */

    /* Sets the time, in seconds since the Unix epoch, in UTC. Returns false
     * if the board has no clock, or the clock did not take it. Call from a
     * task, with the scheduler running. */
    bool BddTargetBoardClock_Set(uint32_t secondsSinceEpoch);

    /* True once the board knows the time. */
    bool BddTargetBoardClock_IsSet(void);

    /* A SolidSyslogClockFunction, so safe to call from any task at any time:
     * the time in UTC, or a zeroed timestamp while the board does not know
     * it. */
    void BddTargetBoardClock_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_CLOCK_H */
