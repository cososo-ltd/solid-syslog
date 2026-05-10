#include "SolidSyslogFreeRtosSysUpTime.h"

#include "FreeRTOS.h"
#include "task.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

uint32_t SolidSyslogFreeRtosSysUpTime_Get(void)
{
    /* uint64 intermediate so the formula stays correct at any
     * configTICK_RATE_HZ; the cast wraps per RFC 3418 TimeTicks. */
    uint64_t hundredths = ((uint64_t) xTaskGetTickCount() * HUNDREDTHS_PER_SECOND) / configTICK_RATE_HZ;
    return (uint32_t) hundredths;
}
