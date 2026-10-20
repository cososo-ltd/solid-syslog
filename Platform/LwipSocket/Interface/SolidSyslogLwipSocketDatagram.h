/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over an lwIP Sockets API socket, for a UdpSender.
 *
 *  What the datagram does through its vtable is the substance:
 *
 *  - Open takes an IPv4 UDP socket from the stack, and answers false if the
 *    stack will not give one.
 *  - SendTo writes the whole payload to the address it is handed, and reports
 *    SENT, OVERSIZE where the stack says the datagram is too long for one
 *    message, or FAILED for every other refusal.
 *  - MaxPayload answers the limit the config states, or the
 *    SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable: lwIP's sockets layer exposes
 *    no path MTU to read back.
 *  - Close releases the socket, and Destroy closes one still open. */
#ifndef SOLIDSYSLOGLWIPSOCKETDATAGRAM_H
#define SOLIDSYSLOGLWIPSOCKETDATAGRAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogUdpPayloadLimitFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** States what only the integrator can know: the IPv4 payload the path
     *  carries, which this stack cannot discover. */
    struct SolidSyslogLwipSocketDatagramConfig
    {
        /** The payload limit in bytes; NULL, or an answer of zero, uses the
         *  SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT tunable. */
        SolidSyslogUdpPayloadLimitFunction GetPayloadLimit;
        void* PayloadLimitContext; /**< Passed back to GetPayloadLimit unchanged; NULL is fine. */
    };

    /** Draw a datagram from the pool; @p config may be NULL. An exhausted pool
     *  falls back to the shared NullDatagram, whose SendTo reports SENT so
     *  undeliverables are dropped rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogLwipSocketDatagram_Create(
        const struct SolidSyslogLwipSocketDatagramConfig* config
    );
    /** Release the pool slot and close the socket. */
    void SolidSyslogLwipSocketDatagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETDATAGRAM_H */
