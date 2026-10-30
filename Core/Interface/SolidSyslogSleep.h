/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The millisecond-sleep callback typedef the integrator injects to drive the
 *  bounded wait-and-retry loops without busy-spinning. */
#ifndef SOLIDSYSLOGSLEEP_H
#define SOLIDSYSLOGSLEEP_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Millisecond sleep callback injected via config. Drives the bounded
     *  wait-and-retry loops that would otherwise busy-spin: the TLS handshake
     *  poll, and the lwIP TCP connect and DNS resolve spins.
     *
     *  It must wait at least @p milliseconds. Those loops read no clock: each
     *  budget is the sum of the sleeps requested, so a sleep that returns early
     *  or only yields shortens it, and one that rounds up to a scheduler tick
     *  lengthens it. On an RTOS, wrap its delay and round a non-zero request up
     *  to one tick.
     *
     *  Injecting it keeps platform sleep APIs out of the library and lets tests
     *  pass a no-op with no conditional compilation. */
    typedef void (*SolidSyslogSleepFunction)(uint32_t milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSLEEP_H */
