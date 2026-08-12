#include "SolidSyslogFreeRtosSysUpTime.h"

#include "FreeRTOS.h"
#include "task.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

uint32_t SolidSyslogFreeRtos_GetSysUpTime(void)
{
    /* Divide the tick count down before scaling by 100 so the intermediate
     * cannot overflow even a 64-bit TickType_t; the whole/remainder split is
     * exact floor division, and the uint32 cast wraps per RFC 3418 TimeTicks.
     * The header states which tick configurations wrap early. */
    uint64_t ticks = (uint64_t) xTaskGetTickCount();
    uint64_t wholeSecondHundredths = (ticks / configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths = ((ticks % configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND) / configTICK_RATE_HZ;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}
