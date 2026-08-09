/** @file
 *  Error codes and Source identity for the PlusTcpAddress adapter. */
#ifndef SOLIDSYSLOGPLUSTCPADDRESSERRORS_H
#define SOLIDSYSLOGPLUSTCPADDRESSERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PlusTcpAddressErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPlusTcpAddressErrors
    {
        SOLIDSYSLOG_PLUSTCP_ADDRESS_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_PLUSTCP_ADDRESS_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_PLUSTCP_ADDRESS_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusTcpAddress. A handler matches by
     *  address (event->Source == &PlusTcpAddressErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPlusTcpAddressErrors. */
    extern const struct SolidSyslogErrorSource PlusTcpAddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPADDRESSERRORS_H */
