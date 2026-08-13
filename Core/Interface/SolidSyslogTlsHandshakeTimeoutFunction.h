/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The callback typedef the integrator installs on a TLS stream config to
 *  supply the bounded-handshake deadline per attempt (runtime-tunable). */
#ifndef SOLIDSYSLOGTLSHANDSHAKETIMEOUTFUNCTION_H
#define SOLIDSYSLOGTLSHANDSHAKETIMEOUTFUNCTION_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Returns the bounded-handshake deadline in milliseconds. Called at the start
     *  of every TLS handshake attempt, so a runtime-tunable value (e.g.
     *  commissioning-time NVRAM read, live web-UI override) takes effect on the
     *  next reconnect without rebuilding or destroying the stream. @p context is
     *  passed through unchanged from the config slot the integrator installed it
     *  on. */
    typedef uint32_t (*SolidSyslogTlsHandshakeTimeoutFunction)(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTLSHANDSHAKETIMEOUTFUNCTION_H */
