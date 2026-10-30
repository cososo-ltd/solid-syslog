/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A FreeRTOS-Plus-TCP destination-address handle wrapping struct
 *  freertos_sockaddr.
 *
 *  A Resolver writes the resolved IPv4 endpoint into it; a Datagram or Stream
 *  reads it back to send. It is a value slot the two sides share, not a vtable
 *  object. */
#ifndef SOLIDSYSLOGPLUSTCPADDRESS_H
#define SOLIDSYSLOGPLUSTCPADDRESS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    /** Draw one per sender from the pool (SOLIDSYSLOG_ADDRESS_POOL_SIZE); on
     *  exhaustion SolidSyslogPlusTcpAddress_Create returns a shared fallback
     *  instead; raise the pool size so that no sender draws it. */
    struct SolidSyslogAddress* SolidSyslogPlusTcpAddress_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPlusTcpAddress_Destroy(struct SolidSyslogAddress * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPADDRESS_H */
