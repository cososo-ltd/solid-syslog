/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosSysUpTime.h"

#include <stdint.h>

#include "cmsis_os2.h"

uint32_t SolidSyslogCmsisRtos_GetSysUpTime(void)
{
    uint32_t ticks = osKernelGetTickCount();
    uint32_t tickFreqHz = osKernelGetTickFreq();

    /* Divide before scaling so the intermediate cannot overflow. */
    return ((ticks / tickFreqHz) * 100U) + (((ticks % tickFreqHz) * 100U) / tickFreqHz);
}
