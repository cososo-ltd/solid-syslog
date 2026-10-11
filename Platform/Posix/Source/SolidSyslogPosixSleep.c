/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixSleep.h"

#include <stdint.h>
#include <time.h>

enum
{
    MILLISECONDS_PER_SECOND = 1000U,
    NANOSECONDS_PER_MILLISECOND = 1000000L
};

void SolidSyslogPosix_Sleep(uint32_t milliseconds)
{
    struct timespec ts = {
        .tv_sec = (time_t) (milliseconds / MILLISECONDS_PER_SECOND),
        .tv_nsec = (long) (milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND
    };
    (void) nanosleep(&ts, NULL);
}
