/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The opaque resolved-destination handle a Resolver writes and a Datagram or
 *  Stream reads; the concrete layout is private to each platform's sources. */
#ifndef SOLIDSYSLOGADDRESS_H
#define SOLIDSYSLOGADDRESS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Opaque resolved-destination handle. Obtained from a per-platform factory
     *  (SolidSyslog{Posix,Winsock,PlusTcp,LwipRaw}Address_Create); a Resolver
     *  writes the resolved address into it and a Datagram or Stream later reads
     *  it back to send. The concrete layout is platform-specific and private to
     *  that platform's sources. */
    struct SolidSyslogAddress;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGADDRESS_H */
