/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over a FreeRTOS-Plus-TCP socket, for a UdpSender.
 *
 *  SendTo wraps FreeRTOS_sendto (unconnected, destination per call) and reports
 *  SENT when the stack accepts the whole datagram, else FAILED. On an ARP-cache
 *  miss it first issues an ARP probe and yields once (~50 ms) for the reply,
 *  because FreeRTOS-Plus-TCP does not queue datagrams while ARP resolves - the
 *  cold-start packet would otherwise be dropped at the IP layer; if the reply
 *  is late the send is left to fail, since UDP is best-effort and retry belongs
 *  in the store-and-forward layer above. MaxPayload answers the limit the
 *  config states, or the SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable: the stack
 *  exposes no path MTU. */
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
