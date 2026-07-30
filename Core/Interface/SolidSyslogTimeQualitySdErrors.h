/** @file
 *  Error codes and Source identity for the TimeQualitySd. */
#ifndef SOLIDSYSLOGTIMEQUALITYSDERRORS_H
#define SOLIDSYSLOGTIMEQUALITYSDERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is TimeQualitySdErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogTimeQualitySdErrors
    {
        TIMEQUALITYSD_ERROR_NULL_CALLBACK,
        TIMEQUALITYSD_ERROR_POOL_EXHAUSTED,
        TIMEQUALITYSD_ERROR_UNKNOWN_DESTROY,
        TIMEQUALITYSD_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a TimeQualitySd. A handler matches by
     *  address (event->Source == &TimeQualitySdErrorSource), then reads
     *  event->Detail as an enum SolidSyslogTimeQualitySdErrors. */
    extern const struct SolidSyslogErrorSource TimeQualitySdErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITYSDERRORS_H */
