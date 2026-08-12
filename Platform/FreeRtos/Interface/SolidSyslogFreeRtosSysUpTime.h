/** @file
 *  The FreeRTOS SolidSyslogSysUpTimeFunction, for the MetaSd structured-data
 *  element. */
#ifndef SOLIDSYSLOGFREERTOSSYSUPTIME_H
#define SOLIDSYSLOGFREERTOSSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second of uptime from xTaskGetTickCount, for the meta-SD
     *  sysUpTime field. Meets the SolidSyslogSysUpTimeFunction contract for a
     *  64-bit TickType_t at any tick rate, and for a 32-bit one whose
     *  configTICK_RATE_HZ divides 100. Elsewhere — the 1000 Hz default among
     *  them — the value wraps to zero early, so supply your own
     *  SolidSyslogSysUpTimeFunction where uptime matters. */
    uint32_t SolidSyslogFreeRtos_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIME_H */
