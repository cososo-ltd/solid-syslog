#include "SolidSyslogFreeRtosSysUpTime.h"

#include "FreeRTOS.h"
#include "task.h"

#include "SolidSyslogMacros.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

/* The uint32 result is a faithful RFC 3418 modulo-2^32 hundredths counter only
 * while every FreeRTOS tick-counter wrap stays phase-continuous mod 2^32. That
 * offset is zero for a 64-bit TickType_t at any tick rate (its wrap is millions
 * of years out, beyond any realistic uptime), or a 32-bit TickType_t whose
 * configTICK_RATE_HZ divides 100 (100/50/25/20/10/5/4/2/1 Hz) — then each tick
 * is an exact whole number of hundredths and the wrap lands cleanly. A 16-bit
 * tick counter, or a 32-bit rate that does not divide 100 (including any rate
 * above 100 Hz), injects a backwards jump at the wrap — refuse to build it
 * rather than ship a subtly wrong value. An integrator outside this envelope
 * should widen TickType_t (configTICK_TYPE_WIDTH_IN_BITS = TICK_TYPE_WIDTH_64_BITS),
 * pick a rate that divides 100, or supply their own SolidSyslogSysUpTimeFunction. */
SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(TickType_t) >= 8U) || ((sizeof(TickType_t) >= 4U) && ((HUNDREDTHS_PER_SECOND % configTICK_RATE_HZ) == 0U)),
    "SolidSyslogFreeRtosSysUpTime needs a 64-bit TickType_t (any tick rate), or a 32-bit TickType_t "
    "with a configTICK_RATE_HZ that divides 100 (100/50/25/20/10/5/4/2/1 Hz), for a faithful RFC "
    "3418 TimeTicks; widen the tick type, use a dividing rate, or supply your own "
    "SolidSyslogSysUpTimeFunction."
);

uint32_t SolidSyslogFreeRtosSysUpTime_Get(void)
{
    /* Divide the tick count down before scaling by 100 so the intermediate
     * cannot overflow even a 64-bit TickType_t; the whole/remainder split is
     * exact floor division, and the uint32 cast wraps per RFC 3418 TimeTicks. */
    uint64_t ticks = (uint64_t) xTaskGetTickCount();
    uint64_t wholeSecondHundredths = (ticks / configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths = ((ticks % configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND) / configTICK_RATE_HZ;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}
