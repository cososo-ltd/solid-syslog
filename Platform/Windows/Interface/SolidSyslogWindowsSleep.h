/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Windows SolidSyslogSleepFunction, for the bounded waits a TLS handshake
 *  or a name-resolution spin needs. */
#ifndef SOLIDSYSLOGWINDOWSSLEEP_H
#define SOLIDSYSLOGWINDOWSSLEEP_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Wraps Sleep so a bounded retry loop (e.g. the TLS handshake) yields to the
     *  scheduler between attempts. */
    void SolidSyslogWindows_Sleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSLEEP_H */
