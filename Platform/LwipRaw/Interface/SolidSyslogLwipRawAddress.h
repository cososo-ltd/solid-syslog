/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An lwIP destination-address handle wrapping an ip_addr_t plus port.
 *
 *  A Resolver writes the resolved endpoint into it; a Datagram or TcpStream
 *  reads it back to send. It is a value slot the two sides share, not a vtable
 *  object. */
#ifndef SOLIDSYSLOGLWIPRAWADDRESS_H
#define SOLIDSYSLOGLWIPRAWADDRESS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    /** Draw one per sender from the pool (SOLIDSYSLOG_ADDRESS_POOL_SIZE); on
     *  exhaustion Create returns a shared TU-private singleton, so integrators
     *  that overflow the pool share that storage and race on it. */
    struct SolidSyslogAddress* SolidSyslogLwipRawAddress_Create(void);
    /** Release the pool slot. */
    void SolidSyslogLwipRawAddress_Destroy(struct SolidSyslogAddress * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWADDRESS_H */
