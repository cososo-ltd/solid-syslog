/** @file
 *  Error codes and Source identity for the WinsockTcpStream adapter. */
#ifndef SOLIDSYSLOGWINSOCKTCPSTREAMERRORS_H
#define SOLIDSYSLOGWINSOCKTCPSTREAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WinsockTcpStreamErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWinsockTcpStreamErrors
    {
        WINSOCKTCPSTREAM_ERROR_POOL_EXHAUSTED,
        WINSOCKTCPSTREAM_ERROR_UNKNOWN_DESTROY,
        WINSOCKTCPSTREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WinsockTcpStream. A handler matches by
     *  address (event->Source == &WinsockTcpStreamErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWinsockTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource WinsockTcpStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAMERRORS_H */
