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

    /** Hundredths of a second of uptime from xTaskGetTickCount, for the
     *  meta-SD sysUpTime field.
     *
     *  Built for a 32- or 64-bit TickType_t; a 16-bit one resolves to no
     *  implementation, so supply your own SolidSyslogSysUpTimeFunction there.
     *  On a 32-bit counter the wraps are counted, which takes a short
     *  critical section - safe from any task, not from an interrupt. */
    uint32_t SolidSyslogFreeRtos_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIME_H */
