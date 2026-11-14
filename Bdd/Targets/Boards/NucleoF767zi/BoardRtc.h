#ifndef BOARD_RTC_H
#define BOARD_RTC_H

#include <stdbool.h>
#include <stdint.h>

/* The STM32F767's real-time clock on the NUCLEO-F767ZI, run from the board's
 * 32.768 kHz crystal and read and set in seconds since the Unix epoch, UTC.
 *
 * The clock is in the part's backup domain, which a reset does not clear, so
 * it keeps the time through a reset and loses it only with the board's power:
 * nothing on the board supplies the domain when that is off.
 *
 * It keeps a calendar in two digits of year, taken here as 2001 to 2099. */

/* Called from the clock's interrupt as each second starts, with that second. */
typedef void (*BoardRtcSecondFunction)(uint32_t secondsSinceEpoch);

/* Starts the crystal if it is not running, which from cold takes a second or
 * two, and has onSecond called every second from then on once the clock has
 * the time. False if the crystal did not start, and then there is no clock:
 * it has no time and refuses to be set. Call once, with interrupts on, before
 * the scheduler starts. */
bool BoardRtc_Start(BoardRtcSecondFunction onSecond);

/* True once the clock has been set, by this boot or by one before it. */
bool BoardRtc_HasTime(void);

/* The second under way and how far into it the clock is. The clock divides
 * its second into 256, so the microseconds are good to four milliseconds.
 * Call with the clock's interrupt held off, or from it. */
void BoardRtc_Read(uint32_t* secondsSinceEpoch, uint32_t* microseconds);

/* Sets the clock, and the second given starts as this returns. False, with
 * the clock left as it was, for a time outside the years it keeps or a clock
 * that did not start. It waits some tens of microseconds on the clock, without
 * a timer, so it may be called with interrupts off - and is to be, where the
 * caller keeps a count of its own in step with the clock. */
bool BoardRtc_Set(uint32_t secondsSinceEpoch);

#endif
