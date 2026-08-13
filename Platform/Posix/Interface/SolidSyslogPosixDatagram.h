/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over a POSIX socket, for a UdpSender.
 *
 *  The datagram connect()s to the destination on its first SendTo (connected
 *  UDP) and turns on path-MTU discovery (IP_PMTUDISC_DO). SendTo then reports
 *  SENT, OVERSIZE (the datagram exceeds the path MTU, EMSGSIZE), or FAILED.
 *  MaxPayload returns the IPv6-safe default until connected, then tracks the
 *  discovered path MTU. */
#ifndef SOLIDSYSLOGPOSIXDATAGRAM_H
#define SOLIDSYSLOGPOSIXDATAGRAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullDatagram, whose SendTo reports SENT so undeliverables are dropped
     *  rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogPosixDatagram_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPosixDatagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXDATAGRAM_H */
