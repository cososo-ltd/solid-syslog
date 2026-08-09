/** @file
 *  Error codes and Source identity for the PosixAddress adapter. */
#ifndef SOLIDSYSLOGPOSIXADDRESSERRORS_H
#define SOLIDSYSLOGPOSIXADDRESSERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixAddressErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPosixAddressErrors
    {
        SOLIDSYSLOG_POSIX_ADDRESS_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_ADDRESS_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_ADDRESS_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixAddress. A handler matches by address
     *  (event->Source == &PosixAddressErrorSource), then reads event->Detail as
     *  an enum SolidSyslogPosixAddressErrors. */
    extern const struct SolidSyslogErrorSource PosixAddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXADDRESSERRORS_H */
