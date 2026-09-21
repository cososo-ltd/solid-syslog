/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A non-blocking TCP stream over an lwIP Sockets API socket, for a
 *  StreamSender or as the byte transport under a TLS stream. */
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
