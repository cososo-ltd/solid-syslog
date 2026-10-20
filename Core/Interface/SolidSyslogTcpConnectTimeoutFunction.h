/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The callback typedef the integrator installs on a TCP stream config to
 *  supply the bounded-connect deadline per attempt (runtime-tunable). */
#ifndef SOLIDSYSLOGTCPCONNECTTIMEOUTFUNCTION_H
#define SOLIDSYSLOGTCPCONNECTTIMEOUTFUNCTION_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

/** The longest deadline a stream honours for one connect attempt, in
 *  milliseconds: five minutes. A getter answering more is bounded to this,
 *  not rejected. The servicing task blocks for the whole of an attempt, and
 *  every stack gives up on an unanswered SYN well inside this, so a longer
 *  value could only stall the task without changing the outcome. The figure
 *  also keeps the conversion to a stack's own units within 32-bit signed
 *  arithmetic on every target. */
#define SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS_MAX 300000U

    /** Returns the bounded-connect deadline in milliseconds, at most
     *  SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS_MAX. Called at the start of every
     *  connect attempt, so a runtime-tunable value (e.g. commissioning-time
     *  NVRAM read, live web-UI override) takes effect on the next reconnect
     *  without rebuilding or destroying the stream. @p context is passed
     *  through unchanged from the config slot the integrator installed it on. */
    typedef uint32_t (*SolidSyslogTcpConnectTimeoutFunction)(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTCPCONNECTTIMEOUTFUNCTION_H */
