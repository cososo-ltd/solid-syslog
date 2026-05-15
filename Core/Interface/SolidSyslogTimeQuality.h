#ifndef SOLIDSYSLOGTIMEQUALITY_H
#define SOLIDSYSLOGTIMEQUALITY_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_SYNC_ACCURACY_OMIT = 0U
    };

    struct SolidSyslogTimeQuality
    {
        bool TzKnown;
        bool IsSynced;
        uint32_t SyncAccuracyMicroseconds; /* SOLIDSYSLOG_SYNC_ACCURACY_OMIT to omit from output */
    };

    typedef void (*SolidSyslogTimeQualityFunction)(struct SolidSyslogTimeQuality* timeQuality);

EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITY_H */
