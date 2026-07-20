/** @file
 *  The FreeRTOS SolidSyslogSysUpTimeFunction, for the MetaSd structured-data
 *  element. */
#ifndef SOLIDSYSLOGFREERTOSSYSUPTIME_H
#define SOLIDSYSLOGFREERTOSSYSUPTIME_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    /** Hundredths of a second of uptime from xTaskGetTickCount, for the meta-SD
     *  sysUpTime field — a faithful RFC 3418 modulo-2^32 TimeTicks counter. A
     *  uint64 intermediate keeps the scaling correct at any configTICK_RATE_HZ.
     *  A compile-time guard rejects tick configurations whose counter would wrap
     *  before the RFC 497-day period: the supported envelope is a 64-bit
     *  TickType_t at any tick rate, or a 32-bit TickType_t with
     *  configTICK_RATE_HZ <= 100. A 16-bit tick counter, or a 32-bit one above
     *  100 Hz, fails to build (widen the tick type, lower the tick rate, or
     *  supply your own SolidSyslogSysUpTimeFunction). */
    uint32_t SolidSyslogFreeRtosSysUpTime_Get(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIME_H */
