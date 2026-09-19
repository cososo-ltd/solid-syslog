/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The FreeRTOS SolidSyslogSysUpTimeFunction, for the MetaSd structured-data
 *  element. */
#ifndef SOLIDSYSLOGFREERTOSSYSUPTIME_H
#define SOLIDSYSLOGFREERTOSSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second of uptime from xTaskGetTickCount, for the meta-SD
     *  sysUpTime field. Meets the SolidSyslogSysUpTimeFunction contract at any
     *  tick rate on a 32-bit TickType_t, and on a 64-bit one, which needs no
     *  help to get there - wrapping at 2^32 hundredths as RFC 3418 TimeTicks
     *  does. A 16-bit TickType_t, which configUSE_16_BIT_TICKS selects, is
     *  not carried past its own wrap: uptime still returns to zero every
     *  65536 ticks there, so supply your own SolidSyslogSysUpTimeFunction
     *  where that matters.
     *
     *  Above 100 Hz the counter reaches its own wrap before those hundredths
     *  do, so how often it has wrapped is carried across calls. Two
     *  consequences for the integrator. This must be reached once per wrap -
     *  about 50 days at the 1000 Hz default - or a rollover goes unseen;
     *  formatting any message reaches it. And because it keeps state it takes
     *  a short critical section, so it is safe to call from any task but not
     *  from an interrupt. */
    uint32_t SolidSyslogFreeRtos_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIME_H */
