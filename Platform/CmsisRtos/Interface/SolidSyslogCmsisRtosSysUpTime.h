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
     *  meta-SD sysUpTime field. */
    uint32_t SolidSyslogCmsisRtos_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCMSISRTOSSYSUPTIME_H */
