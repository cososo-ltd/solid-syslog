/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Clock.h"

#include <stdbool.h>
#include <stdint.h>

#include "vxWorks.h"

#include <time.h>

#include "SolidSyslogTimestamp.h"

enum
{
    TM_YEAR_BASE = 1900,
    NANOSECONDS_PER_MICROSECOND = 1000
};

static inline bool VxWorks64Clock_GetBrokenDownTime(struct timespec* now, struct tm* breakdown);
static inline void VxWorks64Clock_PopulateTimestamp(
    struct SolidSyslogTimestamp* timestamp,
    const struct timespec* now,
    const struct tm* breakdown
);

void SolidSyslogVxWorks64_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    struct timespec now;
    struct tm breakdown;

    *timestamp = (struct SolidSyslogTimestamp) {0};

    if (VxWorks64Clock_GetBrokenDownTime(&now, &breakdown))
    {
        VxWorks64Clock_PopulateTimestamp(timestamp, &now, &breakdown);
    }
}

/* gmtime_r here answers OK or ERROR rather than a pointer. */
static inline bool VxWorks64Clock_GetBrokenDownTime(struct timespec* now, struct tm* breakdown)
{
    return (clock_gettime(CLOCK_REALTIME, now) == OK) && (gmtime_r(&now->tv_sec, breakdown) == OK);
}

static inline void VxWorks64Clock_PopulateTimestamp(
    struct SolidSyslogTimestamp* timestamp,
    const struct timespec* now,
    const struct tm* breakdown
)
{
    timestamp->Year = (uint16_t) (breakdown->tm_year + TM_YEAR_BASE);
    timestamp->Month = (uint8_t) (breakdown->tm_mon + 1);
    timestamp->Day = (uint8_t) breakdown->tm_mday;
    timestamp->Hour = (uint8_t) breakdown->tm_hour;
    timestamp->Minute = (uint8_t) breakdown->tm_min;
    timestamp->Second = (uint8_t) breakdown->tm_sec;
    timestamp->Microsecond = (uint32_t) (now->tv_nsec / NANOSECONDS_PER_MICROSECOND);
}
