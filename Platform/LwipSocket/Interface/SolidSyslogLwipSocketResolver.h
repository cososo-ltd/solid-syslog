/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A destination resolver over lwIP's getaddrinfo, filling the Resolver role
 *  for the Sockets API.
 *
 *  What the resolver does through its vtable is the substance:
 *
 *  - Resolve asks for an IPv4 address and refuses anything else. A result in
 *    another family is reported rather than returned, because the Datagram and
 *    Stream beside it send to a sockaddr_in and an address they cannot use would
 *    resolve, never deliver, and never say why.
 *  - The port is applied to the resolved address, so the caller's port rather
 *    than a service name decides it.
 *  - A lookup that reaches the network blocks the calling task for as long as
 *    the stack's own DNS timeout allows.
 *
 *  Needs an lwIP built with LWIP_SOCKET and LWIP_DNS. Asking for the class
 *  without them is a link error. */
#ifndef SOLIDSYSLOGLWIPSOCKETRESOLVER_H
#define SOLIDSYSLOGLWIPSOCKETRESOLVER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /** Draw a resolver from the pool (SOLIDSYSLOG_RESOLVER_POOL_SIZE); an
     *  exhausted pool falls back to the shared NullResolver. */
    struct SolidSyslogResolver* SolidSyslogLwipSocketResolver_Create(void);
    /** Release the pool slot. */
    void SolidSyslogLwipSocketResolver_Destroy(struct SolidSyslogResolver * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETRESOLVER_H */
