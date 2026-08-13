/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The POSIX SolidSyslogSleepFunction. */
#ifndef SOLIDSYSLOGPOSIXSLEEP_H
#define SOLIDSYSLOGPOSIXSLEEP_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Sleeps for @p milliseconds via nanosleep. It neither performs nor bounds
     *  retries; callers such as the TLS handshake use it to yield between their own
     *  bounded attempts. */
    void SolidSyslogPosix_Sleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXSLEEP_H */
