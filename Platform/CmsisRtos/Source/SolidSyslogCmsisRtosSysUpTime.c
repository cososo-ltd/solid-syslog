/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosSysUpTime.h"

#include <stdint.h>

#include "cmsis_os2.h"

enum
{
    CMSISRTOS_SYS_UP_TIME_HUNDREDTHS_PER_SECOND = 100
};

static uint32_t CmsisRtosSysUpTime_Hundredths(uint32_t ticks, uint32_t tickFreqHz);

uint32_t SolidSyslogCmsisRtos_GetSysUpTime(void)
{
    return CmsisRtosSysUpTime_Hundredths(osKernelGetTickCount(), osKernelGetTickFreq());
}

static uint32_t CmsisRtosSysUpTime_Hundredths(uint32_t ticks, uint32_t tickFreqHz)
{
    /* Divide before scaling so the intermediate cannot overflow. */
    uint32_t wholeSecondHundredths = (ticks / tickFreqHz) * CMSISRTOS_SYS_UP_TIME_HUNDREDTHS_PER_SECOND;
    uint32_t subSecondHundredths =
        ((ticks % tickFreqHz) * CMSISRTOS_SYS_UP_TIME_HUNDREDTHS_PER_SECOND) / tickFreqHz;
    return wholeSecondHundredths + subSecondHundredths;
}
