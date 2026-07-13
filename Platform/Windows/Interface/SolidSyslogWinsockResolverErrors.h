/** @file
 *  Error codes and Source identity for the WinsockResolver adapter. */
#ifndef SOLIDSYSLOGWINSOCKRESOLVERERRORS_H
#define SOLIDSYSLOGWINSOCKRESOLVERERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WinsockResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWinsockResolverErrors
    {
        WINSOCKRESOLVER_ERROR_POOL_EXHAUSTED,
        WINSOCKRESOLVER_ERROR_UNKNOWN_DESTROY,
        WINSOCKRESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WinsockResolver. A handler matches by
     *  address (event->Source == &WinsockResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWinsockResolverErrors. */
    extern const struct SolidSyslogErrorSource WinsockResolverErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKRESOLVERERRORS_H */
