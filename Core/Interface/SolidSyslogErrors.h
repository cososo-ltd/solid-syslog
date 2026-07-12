#ifndef SOLIDSYSLOGERRORS_H
#define SOLIDSYSLOGERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogErrorSource (the
     *  SolidSyslog singleton). A handler reads these off event->Detail after
     *  matching event->Source; the members name their own fault. */
    enum SolidSyslogErrors
    {
        SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG,
        SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER,
        SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER,
        SOLIDSYSLOG_ERROR_CREATE_NULL_STORE,
        SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD,
        SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE,
        SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE,
        SOLIDSYSLOG_ERROR_LOG_INCONSISTENT_SD,
        SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE,
        SOLIDSYSLOG_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by the SolidSyslog singleton. A handler
     *  matches by address (event->Source == &SolidSyslogErrorSource), then reads
     *  event->Detail as an enum SolidSyslogErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGERRORS_H */
