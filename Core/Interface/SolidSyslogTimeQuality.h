#ifndef SOLIDSYSLOGTIMEQUALITY_H
#define SOLIDSYSLOGTIMEQUALITY_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

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
        uint32_t
            SyncAccuracyMicroseconds; /**< syncAccuracy in microseconds; SOLIDSYSLOG_SYNC_ACCURACY_OMIT omits the field. */
    };

    /** Fills @p timeQuality for the current clock state. Installed as the
     *  TimeQualitySd getTimeQuality callback and called once per formatted message. */
    typedef void (*SolidSyslogTimeQualityFunction)(struct SolidSyslogTimeQuality* timeQuality);

EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITY_H */
