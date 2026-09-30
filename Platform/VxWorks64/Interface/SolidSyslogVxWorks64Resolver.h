/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A blocking IPv4 resolver over the VxWorks 6.4 host library.
 *
 *  Resolve takes a dotted literal as it stands, and otherwise looks the name
 *  up with hostGetByName, which consults the host table and, where the image
 *  includes it, the DNS client. The result is written into the destination
 *  SolidSyslogAddress. A failed lookup returns false, so the caller's
 *  unresolved-host error path runs. */
#ifndef SOLIDSYSLOGVXWORKS64RESOLVER_H
#define SOLIDSYSLOGVXWORKS64RESOLVER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullResolver. */
    struct SolidSyslogResolver* SolidSyslogVxWorks64Resolver_Create(void);
    /** Release the pool slot. */
    void SolidSyslogVxWorks64Resolver_Destroy(struct SolidSyslogResolver * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64RESOLVER_H */
