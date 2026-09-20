/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The CMSIS-RTOS2 SolidSyslogSysUpTimeFunction, for the MetaSd structured-data
 *  element. */
#ifndef SOLIDSYSLOGCMSISRTOSSYSUPTIME_H
#define SOLIDSYSLOGCMSISRTOSSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second of uptime from osKernelGetTickCount, for the
     *  meta-SD sysUpTime field.
     *
     *  The kernel tick counter is 32 bits on every CMSIS-RTOS2
     *  implementation, so above 100 Hz it reaches its own wrap before the
     *  2^32 hundredths RFC 3418 allows. The wraps are counted, which takes a
     *  short scheduler lock - safe from any task, not from an interrupt. */
    uint32_t SolidSyslogCmsisRtos_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCMSISRTOSSYSUPTIME_H */
