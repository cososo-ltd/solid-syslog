/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over a FreeRTOS-Plus-TCP socket, for a UdpSender.
 *
 *  SendTo wraps FreeRTOS_sendto (unconnected, destination per call) and reports
 *  SENT when the stack accepts the whole datagram, else FAILED. The stack drops
 *  a datagram whose next hop it is still resolving, so before each send the
 *  datagram confirms the next hop's link address is in the ARP cache, as the
 *  Datagram contract requires: on a miss it sends one ARP request and checks
 *  every tick for up to SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS, and a next hop
 *  still unresolved fails the send and is reported. MaxPayload answers the
 *  limit the config states, or the SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable:
 *  the stack exposes no path MTU. */
#ifndef SOLIDSYSLOGPLUSTCPDATAGRAM_H
#define SOLIDSYSLOGPLUSTCPDATAGRAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogUdpPayloadLimitFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** States what only the integrator can know: the IPv4 payload the path
     *  carries, which this stack cannot discover. */
    struct SolidSyslogPlusTcpDatagramConfig
    {
        /** The payload limit in bytes; NULL, or an answer of zero, uses the
         *  SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable. */
        SolidSyslogUdpPayloadLimitFunction GetPayloadLimit;
        void* PayloadLimitContext; /**< Passed back to GetPayloadLimit unchanged; NULL is fine. */
    };

    /** Draw a datagram from the pool; @p config may be NULL. An exhausted pool
     *  falls back to the shared NullDatagram, whose SendTo reports SENT so
     *  undeliverables are dropped rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogPlusTcpDatagram_Create(const struct SolidSyslogPlusTcpDatagramConfig* config
    );
    /** Release the pool slot and close the socket. */
    void SolidSyslogPlusTcpDatagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPDATAGRAM_H */
