/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Clock.h"

#include <stdint.h>

#include "vxWorks.h"

#include <time.h>

#include "SolidSyslogTimestamp.h"

void SolidSyslogVxWorks64_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    struct timespec now;
    struct tm breakdown;

    *timestamp = (struct SolidSyslogTimestamp) {0};

    if (clock_gettime(CLOCK_REALTIME, &now) == OK)
    {
        (void) gmtime_r(&now.tv_sec, &breakdown);
        timestamp->Year = (uint16_t) (breakdown.tm_year + 1900);
    }
}
