/** @file
 *  Error codes and Source identity for the WinsockAddress adapter. */
#ifndef SOLIDSYSLOGWINSOCKADDRESSERRORS_H
#define SOLIDSYSLOGWINSOCKADDRESSERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WinsockAddressErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWinsockAddressErrors
    {
        WINSOCKADDRESS_ERROR_POOL_EXHAUSTED,
        WINSOCKADDRESS_ERROR_UNKNOWN_DESTROY,
        WINSOCKADDRESS_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WinsockAddress. A handler matches by
     *  address (event->Source == &WinsockAddressErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWinsockAddressErrors. */
    extern const struct SolidSyslogErrorSource WinsockAddressErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKADDRESSERRORS_H */
