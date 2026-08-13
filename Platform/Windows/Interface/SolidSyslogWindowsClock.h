/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Windows SolidSyslogClockFunction, for SolidSyslogConfig.Clock. */
#ifndef SOLIDSYSLOGWINDOWSCLOCK_H
#define SOLIDSYSLOGWINDOWSCLOCK_H

#include "SolidSyslogTimestamp.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Fills @p timestamp from the system wall clock (GetSystemTimeAsFileTime),
     *  broken down to UTC calendar fields with microsecond precision. */
    void SolidSyslogWindows_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSCLOCK_H */
