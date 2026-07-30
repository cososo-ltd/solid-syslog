/** @file
 *  Error codes and Source identity for the StreamSender. */
#ifndef SOLIDSYSLOGSTREAMSENDERERRORS_H
#define SOLIDSYSLOGSTREAMSENDERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is StreamSenderErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogStreamSenderErrors
    {
        STREAMSENDER_ERROR_NULL_CONFIG,
        STREAMSENDER_ERROR_NULL_RESOLVER,
        STREAMSENDER_ERROR_NULL_STREAM,
        STREAMSENDER_ERROR_NULL_ADDRESS,
        STREAMSENDER_ERROR_POOL_EXHAUSTED,
        STREAMSENDER_ERROR_UNKNOWN_DESTROY,
        STREAMSENDER_ERROR_DELIVERY_FAILED,
        STREAMSENDER_ERROR_DELIVERY_RESTORED,
        STREAMSENDER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a StreamSender. A handler matches by
     *  address (event->Source == &StreamSenderErrorSource), then reads
     *  event->Detail as an enum SolidSyslogStreamSenderErrors. */
    extern const struct SolidSyslogErrorSource StreamSenderErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAMSENDERERRORS_H */
