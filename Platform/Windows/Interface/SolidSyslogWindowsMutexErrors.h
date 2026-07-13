/** @file
 *  Error codes and Source identity for the WindowsMutex adapter. */
#ifndef SOLIDSYSLOGWINDOWSMUTEXERRORS_H
#define SOLIDSYSLOGWINDOWSMUTEXERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WindowsMutexErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogWindowsMutexErrors
    {
        WINDOWSMUTEX_ERROR_POOL_EXHAUSTED,
        WINDOWSMUTEX_ERROR_UNKNOWN_DESTROY,
        WINDOWSMUTEX_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WindowsMutex. A handler matches by address
     *  (event->Source == &WindowsMutexErrorSource), then reads event->Detail as an
     *  enum SolidSyslogWindowsMutexErrors. */
    extern const struct SolidSyslogErrorSource WindowsMutexErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSMUTEXERRORS_H */
