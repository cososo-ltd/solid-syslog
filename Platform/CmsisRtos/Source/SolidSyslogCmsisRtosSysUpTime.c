/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosSysUpTime.h"

#include <stdint.h>

#include "cmsis_os2.h"

uint32_t SolidSyslogCmsisRtos_GetSysUpTime(void)
{
    return (osKernelGetTickCount() * 100U) / osKernelGetTickFreq();
}
