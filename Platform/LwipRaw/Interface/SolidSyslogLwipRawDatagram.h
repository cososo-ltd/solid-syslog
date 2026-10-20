/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over the lwIP Raw API, for a UdpSender.
 *
 *  What the datagram does through its vtable is the substance:
 *
 *  - Open makes a udp_new pcb; Close removes it. Both run under the
 *    SolidSyslogLwipRaw_Marshal hop, since they touch lwIP core state.
 *  - SendTo runs the whole send - pbuf alloc, udp_sendto, free - in a single
 *    marshal hop, so a NO_SYS=0 integrator pays one tcpip-thread context switch
 *    per Send rather than three. The pbuf is PBUF_REF: lwIP points at the
 *    caller's buffer instead of copying it, safe because the buffer outlives the
 *    synchronous hop. Reports SENT on udp_sendto success, else FAILED. Cache-miss
 *    recovery is left to lwIP's ARP_QUEUEING.
 *  - MaxPayload answers the limit the config states, or the
 *    SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable: the stack exposes no path MTU. */
#ifndef SOLIDSYSLOGLWIPRAWDATAGRAM_H
#define SOLIDSYSLOGLWIPRAWDATAGRAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogUdpPayloadLimitFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** States what only the integrator can know: the IPv4 payload the path
     *  carries, which this stack cannot discover. */
    struct SolidSyslogLwipRawDatagramConfig
    {
        /** The payload limit in bytes; NULL, or an answer of zero, uses the
         *  SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable. */
        SolidSyslogUdpPayloadLimitFunction GetPayloadLimit;
        void* PayloadLimitContext; /**< Passed back to GetPayloadLimit unchanged; NULL is fine. */
    };

    /** Draw a datagram from the pool; @p config may be NULL. An exhausted pool
     *  falls back to the shared NullDatagram, whose SendTo reports SENT so
     *  undeliverables are dropped rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogLwipRawDatagram_Create(const struct SolidSyslogLwipRawDatagramConfig* config
    );
    /** Release the pool slot; removes the udp pcb. */
    void SolidSyslogLwipRawDatagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAM_H */
