/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixSleep.h"

#include <time.h>

enum
{
    MILLISECONDS_PER_SECOND = 1000,
    NANOSECONDS_PER_MILLISECOND = 1000000L
};

void SolidSyslogPosix_Sleep(int milliseconds)
{
    struct timespec ts = {
        .tv_sec = milliseconds / MILLISECONDS_PER_SECOND,
        .tv_nsec = (long) (milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND
    };
    (void) nanosleep(&ts, NULL);
}
