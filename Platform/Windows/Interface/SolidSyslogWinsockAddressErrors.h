/** @file
 *  Error codes and Source identity for the WinsockAddress adapter. */
#ifndef SOLIDSYSLOGWINSOCKADDRESSERRORS_H
#define SOLIDSYSLOGWINSOCKADDRESSERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WinsockAddressErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWinsockAddressErrors
    {
        SOLIDSYSLOG_WINSOCK_ADDRESS_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_WINSOCK_ADDRESS_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_WINSOCK_ADDRESS_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WinsockAddress. A handler matches by
     *  address (event->Source == &WinsockAddressErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWinsockAddressErrors. */
    extern const struct SolidSyslogErrorSource WinsockAddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKADDRESSERRORS_H */
