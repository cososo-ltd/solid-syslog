/** @file
 *  Error codes and Source identity for the PlusTcpResolver adapter. */
#ifndef SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H
#define SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PlusTcpResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPlusTcpResolverErrors
    {
        PLUSTCPRESOLVER_ERROR_POOL_EXHAUSTED,
        PLUSTCPRESOLVER_ERROR_UNKNOWN_DESTROY,
        PLUSTCPRESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusTcpResolver. A handler matches by
     *  address (event->Source == &PlusTcpResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPlusTcpResolverErrors. */
    extern const struct SolidSyslogErrorSource PlusTcpResolverErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H */
