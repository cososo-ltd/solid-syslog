/** @file
 *  Error codes and Source identity for the PassthroughBuffer. */
#ifndef SOLIDSYSLOGPASSTHROUGHBUFFERERRORS_H
#define SOLIDSYSLOGPASSTHROUGHBUFFERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PassthroughBufferErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPassthroughBufferErrors
    {
        SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_NULL_SENDER,
        SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a PassthroughBuffer. A handler matches
     *  by address (event->Source == &PassthroughBufferErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPassthroughBufferErrors. */
    extern const struct SolidSyslogErrorSource PassthroughBufferErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFERERRORS_H */
