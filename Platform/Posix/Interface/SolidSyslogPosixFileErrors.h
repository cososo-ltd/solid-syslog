/** @file
 *  Error codes and Source identity for the PosixFile adapter. */
#ifndef SOLIDSYSLOGPOSIXFILEERRORS_H
#define SOLIDSYSLOGPOSIXFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixFileErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPosixFileErrors
    {
        POSIXFILE_ERROR_POOL_EXHAUSTED,
        POSIXFILE_ERROR_UNKNOWN_DESTROY,
        POSIXFILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixFile. A handler matches by address
     *  (event->Source == &PosixFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogPosixFileErrors. */
    extern const struct SolidSyslogErrorSource PosixFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXFILEERRORS_H */
