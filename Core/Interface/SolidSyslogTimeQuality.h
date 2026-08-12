/** @file
 *  The clock-quality data (tzKnown / isSynced / syncAccuracy) and the callback
 *  that supplies it, feeding the RFC 5424 "timeQuality" SD-ELEMENT. */
#ifndef SOLIDSYSLOGTIMEQUALITY_H
#define SOLIDSYSLOGTIMEQUALITY_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_SYNC_ACCURACY_OMIT =
            0U /**< SyncAccuracyMicroseconds sentinel: leave syncAccuracy out of the SD-ELEMENT. */
    };

    /** Feeds the RFC 5424 "timeQuality" SD-ELEMENT written by a TimeQualitySd. */
    struct SolidSyslogTimeQuality
    {
        bool TzKnown; /**< Emitted as tzKnown=1/0: is the local timezone known. */
        bool IsSynced; /**< Emitted as isSynced=1/0: is the clock synced to a reliable source. */
        /** syncAccuracy in microseconds; SOLIDSYSLOG_SYNC_ACCURACY_OMIT omits the field.
         *  RFC 5424 §7.1.3 forbids the parameter when isSynced is 0, so a callback
         *  reporting IsSynced false must leave this at SOLIDSYSLOG_SYNC_ACCURACY_OMIT.
         *  The field is written whenever it holds any other value - the pairing is
         *  the caller's to keep until #748 enforces it. §7.1.3 also asks that it be
         *  written only where the accuracy of the time source is actually known. */
        uint32_t SyncAccuracyMicroseconds;
    };

    /** Fills @p timeQuality for the current clock state. Installed as the
     *  TimeQualitySd getTimeQuality callback and called once per formatted message. */
    typedef void (*SolidSyslogTimeQualityFunction)(struct SolidSyslogTimeQuality* timeQuality);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITY_H */
