/** @file
 *  The FreeRTOS SolidSyslogSysUpTimeFunction, for the MetaSd structured-data
 *  element. */
#ifndef SOLIDSYSLOGFREERTOSSYSUPTIME_H
#define SOLIDSYSLOGFREERTOSSYSUPTIME_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    /** Hundredths of a second of uptime from xTaskGetTickCount, for the meta-SD
     *  sysUpTime field. A uint64 intermediate keeps the scaling correct at any
     *  configTICK_RATE_HZ, but the value inherits the FreeRTOS tick counter's
     *  wrap — it resets well before the RFC 3418 2^32-hundredths range (and
     *  quickly under configUSE_16_BIT_TICKS), so it is not a full TimeTicks
     *  counter. */
    uint32_t SolidSyslogFreeRtosSysUpTime_Get(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIME_H */
