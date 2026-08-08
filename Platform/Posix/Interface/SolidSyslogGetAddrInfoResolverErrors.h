/** @file
 *  Error codes and Source identity for the GetAddrInfoResolver adapter. */
#ifndef SOLIDSYSLOGGETADDRINFORESOLVERERRORS_H
#define SOLIDSYSLOGGETADDRINFORESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is GetAddrInfoResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogGetAddrInfoResolverErrors
    {
        GETADDRINFORESOLVER_ERROR_POOL_EXHAUSTED,
        GETADDRINFORESOLVER_ERROR_UNKNOWN_DESTROY,
        GETADDRINFORESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a GetAddrInfoResolver. A handler matches by
     *  address (event->Source == &GetAddrInfoResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogGetAddrInfoResolverErrors. */
    extern const struct SolidSyslogErrorSource GetAddrInfoResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGGETADDRINFORESOLVERERRORS_H */
