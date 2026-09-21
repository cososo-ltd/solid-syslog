/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An lwIP Sockets destination-address handle wrapping struct sockaddr_in.
 *
 *  A Resolver writes the resolved IPv4 endpoint into it; a Datagram or Stream
 *  reads it back to send. It is a value slot the two sides share, not a vtable
 *  object.
 *
 *  Needs an lwIP built with LWIP_SOCKET. Asking for the class without it is a
 *  link error. */
#ifndef SOLIDSYSLOGLWIPSOCKETADDRESS_H
#define SOLIDSYSLOGLWIPSOCKETADDRESS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    /** Draw one per sender from the pool (SOLIDSYSLOG_ADDRESS_POOL_SIZE); on
     *  exhaustion Create returns a shared TU-private singleton, so integrators
     *  that overflow the pool share that storage and race on it. */
    struct SolidSyslogAddress* SolidSyslogLwipSocketAddress_Create(void);
    /** Release the pool slot. */
    void SolidSyslogLwipSocketAddress_Destroy(struct SolidSyslogAddress * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETADDRESS_H */
