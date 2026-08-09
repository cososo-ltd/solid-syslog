/** @file
 *  Error codes and Source identity for the StdAtomicCounter adapter. */
#ifndef SOLIDSYSLOGSTDATOMICCOUNTERERRORS_H
#define SOLIDSYSLOGSTDATOMICCOUNTERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is StdAtomicCounterErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogStdAtomicCounterErrors
    {
        SOLIDSYSLOG_STDATOMIC_COUNTER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_STDATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_STDATOMIC_COUNTER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a StdAtomicCounter. A handler matches by
     *  address (event->Source == &StdAtomicCounterErrorSource), then reads
     *  event->Detail as an enum SolidSyslogStdAtomicCounterErrors. */
    extern const struct SolidSyslogErrorSource StdAtomicCounterErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTDATOMICCOUNTERERRORS_H */
