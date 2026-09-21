/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A non-blocking TCP stream over an lwIP Sockets API socket, for a
 *  StreamSender or as the byte transport under a TLS stream.
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open takes a socket, makes it non-blocking, and connects: immediate where
 *    the stack can answer at once, otherwise a bounded wait up to the config's
 *    GetConnectTimeoutMs (re-read each attempt, so a runtime-tunable value
 *    applies on the next reconnect), with the deferred result read back
 *    afterwards. Every failure closes the socket and reports which step gave
 *    up.
 *  - Send is all-or-nothing and never blocks the service thread: a short write
 *    or any error is taken as a dead connection, so the stream closes itself
 *    and the sender reconnects on its next pass.
 *  - Read answers the bytes read, 0 when nothing has arrived (connection
 *    kept), or tears the connection down on a peer close or a fault.
 *  - Version answers 0 for the stream's lifetime: nothing about a plain socket's
 *    own configuration moves at runtime.
 *
 *  Coalescing is off and keepalive on, with the idle period taken from
 *  SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS; a stack built with
 *  LWIP_TCP_KEEPALIVE takes the probe interval and count too. An option the
 *  stack declines is reported once per Open and the connection still stands. */
#ifndef SOLIDSYSLOGLWIPSOCKETTCPSTREAM_H
#define SOLIDSYSLOGLWIPSOCKETTCPSTREAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStream;

    /** Tunes SolidSyslogLwipSocketTcpStream's bounded connect. */
    struct SolidSyslogLwipSocketTcpStreamConfig
    {
        /** Per-attempt connect deadline in ms; NULL uses the
         *  SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable. */
        SolidSyslogTcpConnectTimeoutFunction GetConnectTimeoutMs;
        void* ConnectTimeoutContext; /**< Passed back to GetConnectTimeoutMs unchanged; NULL is fine. */
    };

    /** Draw a TCP stream from the pool; the config's GetConnectTimeoutMs bounds
     *  the connect. An exhausted pool falls back to the shared NullStream. */
    struct SolidSyslogStream* SolidSyslogLwipSocketTcpStream_Create(
        const struct SolidSyslogLwipSocketTcpStreamConfig* config
    );
    /** Release the pool slot and close the socket. */
    void SolidSyslogLwipSocketTcpStream_Destroy(struct SolidSyslogStream * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETTCPSTREAM_H */
