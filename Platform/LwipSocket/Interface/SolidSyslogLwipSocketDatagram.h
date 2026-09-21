/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  UDP transport over an lwIP Sockets API socket, for a UdpSender. */
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
