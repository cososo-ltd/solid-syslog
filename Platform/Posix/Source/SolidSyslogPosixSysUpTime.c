/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixSysUpTime.h"

#include <time.h>

enum
{
    HUNDREDTHS_PER_SECOND = 100,
    NANOSECONDS_PER_HUNDREDTH = 10000000
};

uint32_t SolidSyslogPosix_GetSysUpTime(void)
{
    struct timespec now;
    uint32_t result = 0;

    if (clock_gettime(CLOCK_BOOTTIME, &now) == 0)
    {
        uint64_t hundredths =
            ((uint64_t) (now.tv_sec) * HUNDREDTHS_PER_SECOND) + ((uint64_t) (now.tv_nsec) / NANOSECONDS_PER_HUNDREDTH);
        result = (uint32_t) hundredths;
    }

    return result;
}
