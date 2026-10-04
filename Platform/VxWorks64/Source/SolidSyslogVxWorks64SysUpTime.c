/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64SysUpTime.h"

#include <stdint.h>

#include "vxWorks.h"

#include "sysLib.h"
#include "tickLib.h"

uint32_t SolidSyslogVxWorks64_GetSysUpTime(void)
{
    return (uint32_t) ((tick64Get() * 100U) / (UINT64) sysClkRateGet());
}
