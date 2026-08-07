/** @file
 *  Error codes and Source identity for the PosixMutex adapter.
 *
 *  @ingroup platform_posix */
#ifndef SOLIDSYSLOGPOSIXMUTEXERRORS_H
#define SOLIDSYSLOGPOSIXMUTEXERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixMutexErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPosixMutexErrors
    {
        POSIXMUTEX_ERROR_POOL_EXHAUSTED,
        POSIXMUTEX_ERROR_UNKNOWN_DESTROY,
        POSIXMUTEX_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixMutex. A handler matches by address
     *  (event->Source == &PosixMutexErrorSource), then reads event->Detail as an
     *  enum SolidSyslogPosixMutexErrors. */
    extern const struct SolidSyslogErrorSource PosixMutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEXERRORS_H */
