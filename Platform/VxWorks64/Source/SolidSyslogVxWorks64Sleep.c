/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Sleep.h"

#include "vxWorks.h"

#include "sysLib.h"
#include "taskLib.h"

void SolidSyslogVxWorks64_Sleep(int milliseconds)
{
    (void) taskDelay((milliseconds * sysClkRateGet()) / 1000);
}
