/** @file
 *  Error codes and Source identity for the LwipRawDnsResolver adapter.
 *
 *  @ingroup platform_lwipraw */
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
        LWIPRAWDNSRESOLVER_ERROR_POOL_EXHAUSTED,
        LWIPRAWDNSRESOLVER_ERROR_UNKNOWN_DESTROY,
        LWIPRAWDNSRESOLVER_ERROR_RESOLVE_TIMEOUT, /**< The bounded async-resolve spin hit its deadline. */
        LWIPRAWDNSRESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a LwipRawDnsResolver. A handler matches by
     *  address (event->Source == &LwipRawDnsResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogLwipRawDnsResolverErrors. */
    extern const struct SolidSyslogErrorSource LwipRawDnsResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H */
