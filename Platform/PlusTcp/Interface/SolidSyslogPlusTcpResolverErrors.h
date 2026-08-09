/** @file
 *  Error codes and Source identity for the PlusTcpResolver adapter. */
#ifndef SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H
#define SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PlusTcpResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPlusTcpResolverErrors
    {
        SOLIDSYSLOG_PLUSTCP_RESOLVER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_PLUSTCP_RESOLVER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_PLUSTCP_RESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusTcpResolver. A handler matches by
     *  address (event->Source == &PlusTcpResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPlusTcpResolverErrors. */
    extern const struct SolidSyslogErrorSource PlusTcpResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H */
