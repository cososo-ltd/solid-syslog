/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The VxWorks 6.4 SolidSyslogSysUpTimeFunction, for MetaSd. */
#ifndef SOLIDSYSLOGVXWORKS64SYSUPTIME_H
#define SOLIDSYSLOGVXWORKS64SYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    uint32_t SolidSyslogVxWorks64_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64SYSUPTIME_H */
