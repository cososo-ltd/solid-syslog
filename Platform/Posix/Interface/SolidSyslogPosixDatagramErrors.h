/** @file
 *  Error codes and Source identity for the PosixDatagram adapter. */
#ifndef SOLIDSYSLOGPOSIXDATAGRAMERRORS_H
#define SOLIDSYSLOGPOSIXDATAGRAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixDatagramErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPosixDatagramErrors
    {
        SOLIDSYSLOG_POSIX_DATAGRAM_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_DATAGRAM_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_DATAGRAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixDatagram. A handler matches by address
     *  (event->Source == &PosixDatagramErrorSource), then reads event->Detail as
     *  an enum SolidSyslogPosixDatagramErrors. */
    extern const struct SolidSyslogErrorSource PosixDatagramErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXDATAGRAMERRORS_H */
