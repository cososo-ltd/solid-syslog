/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the WinsockResolver adapter. */
#ifndef SOLIDSYSLOGWINSOCKRESOLVERERRORS_H
#define SOLIDSYSLOGWINSOCKRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WinsockResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWinsockResolverErrors
    {
        SOLIDSYSLOG_WINSOCK_RESOLVER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_WINSOCK_RESOLVER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_WINSOCK_RESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WinsockResolver. A handler matches by
     *  address (event->Source == &WinsockResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWinsockResolverErrors. */
    extern const struct SolidSyslogErrorSource WinsockResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKRESOLVERERRORS_H */
