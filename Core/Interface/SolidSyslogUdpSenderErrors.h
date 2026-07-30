/** @file
 *  Error codes and Source identity for the UdpSender. */
#ifndef SOLIDSYSLOGUDPSENDERERRORS_H
#define SOLIDSYSLOGUDPSENDERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is UdpSenderErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogUdpSenderErrors
    {
        UDPSENDER_ERROR_NULL_CONFIG,
        UDPSENDER_ERROR_NULL_RESOLVER,
        UDPSENDER_ERROR_NULL_DATAGRAM,
        UDPSENDER_ERROR_NULL_ADDRESS,
        UDPSENDER_ERROR_NULL_ENDPOINT,
        UDPSENDER_ERROR_SEND_NULL_BUFFER,
        UDPSENDER_ERROR_POOL_EXHAUSTED,
        UDPSENDER_ERROR_UNKNOWN_DESTROY,
        UDPSENDER_ERROR_DELIVERY_FAILED,
        UDPSENDER_ERROR_DELIVERY_RESTORED,
        UDPSENDER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a UdpSender. A handler matches by
     *  address (event->Source == &UdpSenderErrorSource), then reads
     *  event->Detail as an enum SolidSyslogUdpSenderErrors. */
    extern const struct SolidSyslogErrorSource UdpSenderErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGUDPSENDERERRORS_H */
