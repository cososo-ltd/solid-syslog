/** @file
 *  Error codes and Source identity for the FreeRtosMutex adapter. */
#ifndef SOLIDSYSLOGFREERTOSMUTEXERRORS_H
#define SOLIDSYSLOGFREERTOSMUTEXERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is FreeRtosMutexErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogFreeRtosMutexErrors
    {
        FREERTOSMUTEX_ERROR_POOL_EXHAUSTED,
        FREERTOSMUTEX_ERROR_UNKNOWN_DESTROY,
        FREERTOSMUTEX_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a FreeRtosMutex. A handler matches by
     *  address (event->Source == &FreeRtosMutexErrorSource), then reads
     *  event->Detail as an enum SolidSyslogFreeRtosMutexErrors. */
    extern const struct SolidSyslogErrorSource FreeRtosMutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSMUTEXERRORS_H */
