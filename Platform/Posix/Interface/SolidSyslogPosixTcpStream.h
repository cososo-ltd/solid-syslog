/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A non-blocking TCP stream over a POSIX socket, for a StreamSender or as the
 *  byte transport under a TLS stream.
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open connects without blocking: immediate on loopback, otherwise a
 *    select()-bounded wait up to the config's GetConnectTimeoutMs (re-read each
 *    attempt, so a runtime-tunable value applies on the next reconnect); a
 *    refused or unreachable peer fails fast.
 *  - Send is all-or-nothing and never blocks the service thread: a short write
 *    or any error is taken as a dead connection, so the stream closes itself and
 *    the sender reconnects on its next pass.
 *  - Read returns the bytes read, 0 for would-block (EAGAIN, connection kept), or
 *    tears the connection down on peer close or error.
 *
 *  TCP_NODELAY is on, and kernel keepalive (idle ~45s, then 4 x 10s probes) plus
 *  TCP_USER_TIMEOUT (30s) on unacked writes surface a wedged peer as a failed
 *  Send/Read rather than a hung service thread. */
#ifndef SOLIDSYSLOGPOSIXTCPSTREAM_H
#define SOLIDSYSLOGPOSIXTCPSTREAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStream;

    /** Tunes SolidSyslogPosixTcpStream's bounded connect. */
    struct SolidSyslogPosixTcpStreamConfig
    {
        /** Per-attempt connect deadline in ms; NULL uses the
         *  SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable. */
        SolidSyslogTcpConnectTimeoutFunction GetConnectTimeoutMs;
        void* ConnectTimeoutContext; /**< Passed back to GetConnectTimeoutMs unchanged; NULL is fine. */
    };

    /** Draw a TCP stream from the pool; the config's GetConnectTimeoutMs bounds
     *  the connect (see the file overview for the stream's behaviour). An
     *  exhausted pool (default size 2, for the plain-TCP + TLS-under-TCP pair)
     *  falls back to the shared NullStream. */
    struct SolidSyslogStream* SolidSyslogPosixTcpStream_Create(const struct SolidSyslogPosixTcpStreamConfig* config);
    /** Release the pool slot and close the socket. */
    void SolidSyslogPosixTcpStream_Destroy(struct SolidSyslogStream * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXTCPSTREAM_H */
