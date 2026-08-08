/** @file
 *  Error codes and Source identity for the LwipRawResolver adapter.
 *
 *  @ingroup platform_lwipraw */
#ifndef SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H
#define SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is LwipRawResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogLwipRawResolverErrors
    {
        LWIPRAWRESOLVER_ERROR_POOL_EXHAUSTED,
        LWIPRAWRESOLVER_ERROR_UNKNOWN_DESTROY,
        LWIPRAWRESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a LwipRawResolver. A handler matches by
     *  address (event->Source == &LwipRawResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogLwipRawResolverErrors. */
    extern const struct SolidSyslogErrorSource LwipRawResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H */
