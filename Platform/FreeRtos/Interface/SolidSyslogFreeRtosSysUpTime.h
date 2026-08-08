/** @file
 *  The FreeRTOS SolidSyslogSysUpTimeFunction, for the MetaSd structured-data
 *  element.
 *
 *  @ingroup platform_freertos */
#ifndef SOLIDSYSLOGFREERTOSSYSUPTIME_H
#define SOLIDSYSLOGFREERTOSSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second of uptime from xTaskGetTickCount, for the meta-SD
     *  sysUpTime field — a faithful RFC 3418 modulo-2^32 TimeTicks counter. A
     *  compile-time guard rejects tick configurations whose counter would wrap
     *  non-continuously mod 2^32. Supported envelope: a 64-bit TickType_t at any
     *  tick rate (faithful over any realistic uptime — its wrap is millions of
     *  years out), or a 32-bit TickType_t whose configTICK_RATE_HZ divides 100
     *  (100/50/25/20/10/5/4/2/1 Hz — faithful for the counter's whole lifetime).
     *  A 16-bit tick counter, or a 32-bit rate that does not divide 100
     *  (including any rate above 100 Hz), fails to build (widen the tick type,
     *  pick a dividing rate, or supply your own SolidSyslogSysUpTimeFunction). */
    uint32_t SolidSyslogFreeRtosSysUpTime_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIME_H */
