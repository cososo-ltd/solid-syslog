#include "SolidSyslogFreeRtosSysUpTime.h"

#include "FreeRTOS.h"
#include "task.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

/* The uint32 result is a faithful RFC 3418 modulo-2^32 hundredths counter only
 * while the FreeRTOS tick counter itself does not wrap before the RFC 497-day
 * period. That holds for a 64-bit TickType_t at any tick rate, or a 32-bit
 * TickType_t while configTICK_RATE_HZ <= HUNDREDTHS_PER_SECOND (at 100 Hz the
 * tick wrap lands exactly on the hundredths wrap). A 16-bit tick counter, or a
 * 32-bit one above 100 Hz, wraps sooner and would report an uptime that jumps
 * backwards early — refuse to build it rather than ship a subtly wrong value.
 * An integrator outside this envelope should widen TickType_t
 * (configTICK_TYPE_WIDTH_IN_BITS = TICK_TYPE_WIDTH_64_BITS), lower
 * configTICK_RATE_HZ to <= 100, or supply their own SolidSyslogSysUpTimeFunction. */
_Static_assert(
    (sizeof(TickType_t) >= 8U) || ((sizeof(TickType_t) >= 4U) && (configTICK_RATE_HZ <= HUNDREDTHS_PER_SECOND)),
    "SolidSyslogFreeRtosSysUpTime needs a 64-bit TickType_t, or a 32-bit TickType_t with "
    "configTICK_RATE_HZ <= 100, for a faithful RFC 3418 TimeTicks; widen the tick type, lower the "
    "tick rate, or supply your own SolidSyslogSysUpTimeFunction."
);

uint32_t SolidSyslogFreeRtosSysUpTime_Get(void)
{
    /* uint64 intermediate so the formula stays correct at any
     * configTICK_RATE_HZ; the cast wraps per RFC 3418 TimeTicks. */
    uint64_t hundredths = ((uint64_t) xTaskGetTickCount() * HUNDREDTHS_PER_SECOND) / configTICK_RATE_HZ;
    return (uint32_t) hundredths;
}
