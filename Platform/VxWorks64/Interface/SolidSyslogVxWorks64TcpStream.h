/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A TCP stream over a VxWorks 6.4 socket, for a StreamSender. */
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

    struct SolidSyslogStream* SolidSyslogVxWorks64TcpStream_Create(
        const struct SolidSyslogVxWorks64TcpStreamConfig* config
    );
    void SolidSyslogVxWorks64TcpStream_Destroy(struct SolidSyslogStream * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64TCPSTREAM_H */
