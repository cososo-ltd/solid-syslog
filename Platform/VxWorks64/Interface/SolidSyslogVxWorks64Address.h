/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A VxWorks 6.4 destination-address handle wrapping struct sockaddr_in.
 *
 *  A Resolver writes the resolved IPv4 endpoint into it; a Datagram reads it
 *  back to send. It is a value slot the two sides share, not a vtable object. */
#ifndef SOLIDSYSLOGVXWORKS64ADDRESS_H
#define SOLIDSYSLOGVXWORKS64ADDRESS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    /** Draw one per sender from the pool (SOLIDSYSLOG_ADDRESS_POOL_SIZE); on
     *  exhaustion Create returns a shared TU-private singleton, so integrators
     *  that overflow the pool share that storage and race on it. */
    struct SolidSyslogAddress* SolidSyslogVxWorks64Address_Create(void);
    /** Release the pool slot. */
    void SolidSyslogVxWorks64Address_Destroy(struct SolidSyslogAddress * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64ADDRESS_H */
