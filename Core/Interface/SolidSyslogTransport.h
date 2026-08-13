/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The transport selector enum (UDP / TCP) and the IANA default-port
 *  convenience constants. */
#ifndef SOLIDSYSLOGTRANSPORT_H
#define SOLIDSYSLOGTRANSPORT_H

/** IANA-registered default destination ports per transport. Convenience
 *  constants only, not applied automatically: the integrator supplies the port
 *  to the endpoint callback and may pick any value. */
enum
{
    SOLIDSYSLOG_UDP_DEFAULT_PORT = 514, /**< RFC 5426. */
    SOLIDSYSLOG_TCP_DEFAULT_PORT = 601, /**< RFC 6587 §3.2 / IANA syslog over TCP. */
    SOLIDSYSLOG_TLS_DEFAULT_PORT = 6514 /**< RFC 5425 §4.2 / IANA syslog-tls. */
};

enum SolidSyslogTransport
{
    SOLIDSYSLOG_TRANSPORT_UDP,
    SOLIDSYSLOG_TRANSPORT_TCP
};

#endif /* SOLIDSYSLOGTRANSPORT_H */
