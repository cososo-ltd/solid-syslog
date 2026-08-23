/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the LwipRawDnsResolver adapter. */
#ifndef SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H
#define SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is LwipRawDnsResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogLwipRawDnsResolverErrors
    {
        SOLIDSYSLOG_LWIPRAW_DNS_RESOLVER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_LWIPRAW_DNS_RESOLVER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_LWIPRAW_DNS_RESOLVER_ERROR_RESOLVE_TIMEOUT, /**< The bounded async-resolve spin hit its deadline. */
        SOLIDSYSLOG_LWIPRAW_DNS_RESOLVER_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_LWIPRAW_DNS_RESOLVER_ERROR_NULL_SLEEP,
        SOLIDSYSLOG_LWIPRAW_DNS_RESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a LwipRawDnsResolver. A handler matches by
     *  address (event->Source == &LwipRawDnsResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogLwipRawDnsResolverErrors. */
    extern const struct SolidSyslogErrorSource LwipRawDnsResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H */
