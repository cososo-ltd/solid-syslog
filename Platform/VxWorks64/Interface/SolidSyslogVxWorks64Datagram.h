/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over a VxWorks 6.4 socket, for a UdpSender.
 *
 *  Each record goes out through an unconnected sendto. SendTo reports SENT,
 *  OVERSIZE (EMSGSIZE), or FAILED. MaxPayload returns the unknown-path
 *  payload: the stack offers no path-MTU query for UDP. */
#ifndef SOLIDSYSLOGVXWORKS64DATAGRAM_H
#define SOLIDSYSLOGVXWORKS64DATAGRAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** Create takes no config; an exhausted pool falls back to
     *  SolidSyslogNullDatagram. */
    struct SolidSyslogDatagram* SolidSyslogVxWorks64Datagram_Create(void);
    /** Release the pool slot. */
    void SolidSyslogVxWorks64Datagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64DATAGRAM_H */
