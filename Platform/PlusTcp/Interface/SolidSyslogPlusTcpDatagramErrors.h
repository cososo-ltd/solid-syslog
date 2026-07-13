/** @file
 *  Error codes and Source identity for the PlusTcpDatagram adapter. */
#ifndef SOLIDSYSLOGPLUSTCPDATAGRAMERRORS_H
#define SOLIDSYSLOGPLUSTCPDATAGRAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PlusTcpDatagramErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPlusTcpDatagramErrors
    {
        PLUSTCPDATAGRAM_ERROR_POOL_EXHAUSTED,
        PLUSTCPDATAGRAM_ERROR_UNKNOWN_DESTROY,
        PLUSTCPDATAGRAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusTcpDatagram. A handler matches by
     *  address (event->Source == &PlusTcpDatagramErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPlusTcpDatagramErrors. */
    extern const struct SolidSyslogErrorSource PlusTcpDatagramErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPDATAGRAMERRORS_H */
