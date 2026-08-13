/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A blocking DNS resolver over POSIX getaddrinfo.
 *
 *  Resolve looks up the endpoint host as an IPv4 address (AF_INET) through a
 *  synchronous getaddrinfo call and writes it into the destination
 *  SolidSyslogAddress; the requested transport selects the socktype hint. A
 *  failed lookup returns false, so the caller's unresolved-host error path
 *  runs. */
#ifndef SOLIDSYSLOGPOSIXRESOLVER_H
#define SOLIDSYSLOGPOSIXRESOLVER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullResolver. */
    struct SolidSyslogResolver* SolidSyslogPosixResolver_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPosixResolver_Destroy(struct SolidSyslogResolver * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXRESOLVER_H */
