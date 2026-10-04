/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The VxWorks 6.4 SolidSyslogClockFunction, for SolidSyslogConfig.Clock. */
#ifndef SOLIDSYSLOGVXWORKS64CLOCK_H
#define SOLIDSYSLOGVXWORKS64CLOCK_H

#include "SolidSyslogExternC.h"

struct SolidSyslogTimestamp;

SOLIDSYSLOG_EXTERN_C_BEGIN

    void SolidSyslogVxWorks64_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64CLOCK_H */
