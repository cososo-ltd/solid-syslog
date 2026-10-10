/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* On its own so that <time.h> is all this file includes beside the kernel's
 * base header: the host tests compile it against a stand-in for the VxWorks
 * time.h, which no host C library header can share a file with. */

#include "vxWorks.h"

#include "BddTargetVxWorks64Clock.h"

#include <stdbool.h>
#include <time.h>

bool BddTargetVxWorks64Clock_Set(unsigned long seconds)
{
    struct timespec now = {0};

    now.tv_sec = (time_t) seconds;
    return clock_settime(CLOCK_REALTIME, &now) == OK;
}
