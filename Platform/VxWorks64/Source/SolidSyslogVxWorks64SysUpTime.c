/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64SysUpTime.h"

#include <stdint.h>

#include "vxWorks.h"

#include "sysLib.h"
#include "tickLib.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

/* Scaled in 64 bits, so only the result wraps: the tick count would overflow
 * in 32. */
uint32_t SolidSyslogVxWorks64_GetSysUpTime(void)
{
    UINT64 hundredths = (tick64Get() * (UINT64) HUNDREDTHS_PER_SECOND) / (UINT64) sysClkRateGet();
    return (uint32_t) hundredths;
}
