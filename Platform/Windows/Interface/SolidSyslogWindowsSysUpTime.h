/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Windows SolidSyslogSysUpTimeFunction, for MetaSd. */
#ifndef SOLIDSYSLOGWINDOWSSYSUPTIME_H
#define SOLIDSYSLOGWINDOWSSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second since boot from GetTickCount64, as RFC 3418
     *  sysUpTime; wraps modulo 2^32 per the TimeTicks contract. */
    uint32_t SolidSyslogWindows_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSYSUPTIME_H */
