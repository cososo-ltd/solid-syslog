/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Sleep.h"

#include <stdint.h>

#include "vxWorks.h"

#include "sysLib.h"
#include "taskLib.h"

enum
{
    MILLISECONDS_PER_SECOND = 1000
};

void SolidSyslogVxWorks64_Sleep(int milliseconds)
{
    int64_t ticks = 0;

    if (milliseconds > 0)
    {
        ticks = (((int64_t) milliseconds * (int64_t) sysClkRateGet()) + (MILLISECONDS_PER_SECOND - 1)) /
                MILLISECONDS_PER_SECOND;
    }
    (void) taskDelay((int) ticks);
}
