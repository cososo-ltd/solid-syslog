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
 *  - MaxPayload answers the unknown-path payload for IPv4: lwIP's sockets
 *    layer exposes no path MTU to read back.
 *  - Close releases the socket, and Destroy closes one still open. */
#ifndef SOLIDSYSLOGLWIPSOCKETDATAGRAM_H
#define SOLIDSYSLOGLWIPSOCKETDATAGRAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullDatagram, whose SendTo reports SENT so undeliverables are dropped
     *  rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogLwipSocketDatagram_Create(void);
    /** Release the pool slot and close the socket. */
    void SolidSyslogLwipSocketDatagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETDATAGRAM_H */
