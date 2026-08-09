/** @file
 *  Error codes and Source identity for the MetaSd. */
#ifndef SOLIDSYSLOGMETASDERRORS_H
#define SOLIDSYSLOGMETASDERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is MetaSdErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogMetaSdErrors
    {
        SOLIDSYSLOG_META_SD_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_META_SD_ERROR_NULL_COUNTER,
        SOLIDSYSLOG_META_SD_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_META_SD_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_META_SD_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a MetaSd. A handler matches by address
     *  (event->Source == &MetaSdErrorSource), then reads event->Detail as an
     *  enum SolidSyslogMetaSdErrors. */
    extern const struct SolidSyslogErrorSource MetaSdErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMETASDERRORS_H */
