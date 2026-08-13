/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

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
        /** Emitted as syncAccuracy: how closely the clock is synced, in microseconds,
         *  when IsSynced is true. Set SOLIDSYSLOG_SYNC_ACCURACY_OMIT when it is false,
         *  or when the accuracy is unknown, to omit the field. RFC 5424 §7.1.3. */
        uint32_t SyncAccuracyMicroseconds;
    };

    /** Fills @p timeQuality for the current clock state. Installed as the
     *  TimeQualitySd getTimeQuality callback and called once per formatted message. */
    typedef void (*SolidSyslogTimeQualityFunction)(struct SolidSyslogTimeQuality* timeQuality);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITY_H */
