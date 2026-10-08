/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A TCP stream over a VxWorks 6.4 socket, for a StreamSender.
 *
 *  - Open connects with connectWithTimeout, bounded by the config's
 *    GetConnectTimeoutMs (re-read each attempt, so a runtime-tunable value
 *    applies on the next reconnect). A refused or unreachable peer fails as soon
 *    as the stack says so. TCP_NODELAY and SO_KEEPALIVE are set once the
 *    connection stands; the platform page covers keepalive timing.
 *  - Send never waits: it peeks with MSG_PEEK | MSG_DONTWAIT before sending,
 *    then sends with MSG_DONTWAIT. A closed peer, a short write or any error
 *    closes the stream, and the sender reconnects on its next pass.
 *  - Read never waits either. It returns the bytes that arrived, 0 when nothing
 *    has (EWOULDBLOCK or EAGAIN, connection kept), or closes the stream on the
 *    peer's close or an error. */
#ifndef SOLIDSYSLOGVXWORKS64TCPSTREAM_H
#define SOLIDSYSLOGVXWORKS64TCPSTREAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStream;

    /** Tunes SolidSyslogVxWorks64TcpStream's bounded connect. */
    struct SolidSyslogVxWorks64TcpStreamConfig
    {
        /** Per-attempt connect deadline in ms; NULL uses the
         *  SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable. */
        SolidSyslogTcpConnectTimeoutFunction GetConnectTimeoutMs;
        void* ConnectTimeoutContext; /**< Passed back to GetConnectTimeoutMs unchanged; NULL is fine. */
    };

    /** Draw a TCP stream from the pool; a NULL config, or one without a getter,
     *  bounds the connect by the tunable. An exhausted pool falls back to the
     *  shared NullStream. */
    struct SolidSyslogStream* SolidSyslogVxWorks64TcpStream_Create(
        const struct SolidSyslogVxWorks64TcpStreamConfig* config
    );
    /** Release the pool slot and close the socket. */
    void SolidSyslogVxWorks64TcpStream_Destroy(struct SolidSyslogStream * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64TCPSTREAM_H */
