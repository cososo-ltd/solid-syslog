/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over a VxWorks 6.4 socket, for a UdpSender.
 *
 *  Each record goes out through an unconnected sendto. SendTo reports SENT,
 *  OVERSIZE (EMSGSIZE), or FAILED. MaxPayload answers the limit the config
 *  states, or the SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable: the stack offers
 *  no path-MTU query for UDP. */
#ifndef SOLIDSYSLOGVXWORKS64DATAGRAM_H
#define SOLIDSYSLOGVXWORKS64DATAGRAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogUdpPayloadLimitFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** States what only the integrator can know: the IPv4 payload the path
     *  carries, which this stack cannot discover. */
    struct SolidSyslogVxWorks64DatagramConfig
    {
        /** The payload limit in bytes; NULL, or an answer of zero, uses the
         *  SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable. */
        SolidSyslogUdpPayloadLimitFunction GetPayloadLimit;
        void* PayloadLimitContext; /**< Passed back to GetPayloadLimit unchanged; NULL is fine. */
    };

    /** Draw a datagram from the pool; @p config may be NULL. An exhausted pool
     *  falls back to the shared NullDatagram, whose SendTo reports SENT so
     *  undeliverables are dropped rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogVxWorks64Datagram_Create(
        const struct SolidSyslogVxWorks64DatagramConfig* config
    );
    /** Release the pool slot. */
    void SolidSyslogVxWorks64Datagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64DATAGRAM_H */
