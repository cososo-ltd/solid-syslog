/** @file
 *  Error codes and Source identity for the PlusTcpTcpStream adapter. */
#ifndef SOLIDSYSLOGPLUSTCPTCPSTREAMERRORS_H
#define SOLIDSYSLOGPLUSTCPTCPSTREAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PlusTcpTcpStreamErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPlusTcpTcpStreamErrors
    {
        PLUSTCPTCPSTREAM_ERROR_POOL_EXHAUSTED,
        PLUSTCPTCPSTREAM_ERROR_UNKNOWN_DESTROY,
        PLUSTCPTCPSTREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusTcpTcpStream. A handler matches by
     *  address (event->Source == &PlusTcpTcpStreamErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPlusTcpTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource PlusTcpTcpStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPTCPSTREAMERRORS_H */
