/** @file
 *  Error codes and Source identity for the PosixResolver adapter. */
#ifndef SOLIDSYSLOGPOSIXRESOLVERERRORS_H
#define SOLIDSYSLOGPOSIXRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPosixResolverErrors
    {
        POSIXRESOLVER_ERROR_POOL_EXHAUSTED,
        POSIXRESOLVER_ERROR_UNKNOWN_DESTROY,
        POSIXRESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixResolver. A handler matches by
     *  address (event->Source == &PosixResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPosixResolverErrors. */
    extern const struct SolidSyslogErrorSource PosixResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXRESOLVERERRORS_H */
