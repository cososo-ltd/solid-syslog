/** @file
 *  Error codes and Source identity for the WinsockDatagram adapter. */
#ifndef SOLIDSYSLOGWINSOCKDATAGRAMERRORS_H
#define SOLIDSYSLOGWINSOCKDATAGRAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WinsockDatagramErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWinsockDatagramErrors
    {
        WINSOCKDATAGRAM_ERROR_POOL_EXHAUSTED,
        WINSOCKDATAGRAM_ERROR_UNKNOWN_DESTROY,
        WINSOCKDATAGRAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WinsockDatagram. A handler matches by
     *  address (event->Source == &WinsockDatagramErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWinsockDatagramErrors. */
    extern const struct SolidSyslogErrorSource WinsockDatagramErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKDATAGRAMERRORS_H */
