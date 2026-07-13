/** @file
 *  Error codes and Source identity for the WindowsAtomicCounter adapter. */
#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WindowsAtomicCounterErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWindowsAtomicCounterErrors
    {
        WINDOWSATOMICCOUNTER_ERROR_POOL_EXHAUSTED,
        WINDOWSATOMICCOUNTER_ERROR_UNKNOWN_DESTROY,
        WINDOWSATOMICCOUNTER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WindowsAtomicCounter. A handler matches by
     *  address (event->Source == &WindowsAtomicCounterErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWindowsAtomicCounterErrors. */
    extern const struct SolidSyslogErrorSource WindowsAtomicCounterErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H */
