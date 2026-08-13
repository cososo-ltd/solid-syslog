/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The POSIX SolidSyslogClockFunction, for SolidSyslogConfig.Clock. */
#ifndef SOLIDSYSLOGPOSIXCLOCK_H
#define SOLIDSYSLOGPOSIXCLOCK_H

#include "SolidSyslogExternC.h"

struct SolidSyslogTimestamp;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Fills @p timestamp from the system real-time clock (CLOCK_REALTIME). */
    void SolidSyslogPosix_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXCLOCK_H */
