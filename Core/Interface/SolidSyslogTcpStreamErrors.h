/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable detail codes for the TCP-stream role, shared by every TCP backend. */
#ifndef SOLIDSYSLOGTCPSTREAMERRORS_H
#define SOLIDSYSLOGTCPSTREAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Detail codes a TCP stream reports through event->Detail, shared by every
     *  TCP backend whichever stack provides it. A handler that matches on these
     *  reacts to a fault identically whichever stack produced it, and keeps
     *  working when the backend is swapped.
     *
     *  event->Source still names the backend that spoke, for diagnosis. Which of
     *  these codes a given backend can raise is a property of that backend and is
     *  stated on its own page; a code it never raises is simply one a handler
     *  never sees. */
    enum SolidSyslogTcpStreamErrors
    {
        SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY,
        /* Wiring the stream cannot work without. */
        SOLIDSYSLOG_TCP_STREAM_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_TCP_STREAM_ERROR_NULL_SLEEP,
        SOLIDSYSLOG_TCP_STREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTCPSTREAMERRORS_H */
