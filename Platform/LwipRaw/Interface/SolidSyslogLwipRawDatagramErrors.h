/** @file
 *  Error codes and Source identity for the LwipRawDatagram adapter. */
#ifndef SOLIDSYSLOGLWIPRAWDATAGRAMERRORS_H
#define SOLIDSYSLOGLWIPRAWDATAGRAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is LwipRawDatagramErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogLwipRawDatagramErrors
    {
        LWIPRAWDATAGRAM_ERROR_POOL_EXHAUSTED,
        LWIPRAWDATAGRAM_ERROR_UNKNOWN_DESTROY,
        LWIPRAWDATAGRAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a LwipRawDatagram. A handler matches by
     *  address (event->Source == &LwipRawDatagramErrorSource), then reads
     *  event->Detail as an enum SolidSyslogLwipRawDatagramErrors. */
    extern const struct SolidSyslogErrorSource LwipRawDatagramErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAMERRORS_H */
