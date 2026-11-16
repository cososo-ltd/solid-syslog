#ifndef BOARD_RTC_H
#define BOARD_RTC_H

#include <stdbool.h>
#include <stdint.h>

/* The STM32F767's real-time clock on the NUCLEO-F767ZI, run from the board's
 * 32.768 kHz crystal and read and set in seconds since the Unix epoch, UTC.
 *
 * The clock is in the part's backup domain, which a reset does not clear, so
 * it keeps the time through a reset and loses it only when the board loses
 * power: the board has no backup supply.
 *
 * It keeps a calendar in two digits of year, taken here as 2001 to 2099. */

/* Called from the clock's interrupt as each second starts, with that second. */
typedef void (*BoardRtcSecondFunction)(uint32_t secondsSinceEpoch);

/* Starts the crystal if it is not running, which from cold takes a second or
 * two, and calls onSecond every second once the clock has the time. False if
 * the crystal did not start, or the clock cannot be run from it or made to
 * interrupt. The clock then has no time and refuses to be set. Call once,
 * with interrupts on, before the scheduler starts. */
bool BoardRtc_Start(BoardRtcSecondFunction onSecond);

/* True once the clock has been set, in this boot or an earlier one. */
bool BoardRtc_HasTime(void);

/* The current second and how far into it the clock is. The clock divides its
 * second into 256, so the microseconds are good to four milliseconds. Call
 * from the clock's interrupt, or with it masked. */
void BoardRtc_Read(uint32_t* secondsSinceEpoch, uint32_t* microseconds);

/* Sets the clock; the second given starts as this returns. False, with the
 * clock unchanged, for a time outside the years it keeps or a clock that did
 * not start. It waits some tens of microseconds on the clock without a timer,
 * so it may be called with interrupts off, and must be where the caller keeps
 * a count of its own in step with the clock. */
bool BoardRtc_Set(uint32_t secondsSinceEpoch);

#endif
