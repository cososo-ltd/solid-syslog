/** @file
 *  Error codes and Source identity for the PosixTcpStream adapter. */
#ifndef SOLIDSYSLOGPOSIXTCPSTREAMERRORS_H
#define SOLIDSYSLOGPOSIXTCPSTREAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixTcpStreamErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPosixTcpStreamErrors
    {
        SOLIDSYSLOG_POSIX_TCP_STREAM_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_TCP_STREAM_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_TCP_STREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixTcpStream. A handler matches by
     *  address (event->Source == &PosixTcpStreamErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPosixTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource PosixTcpStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXTCPSTREAMERRORS_H */
