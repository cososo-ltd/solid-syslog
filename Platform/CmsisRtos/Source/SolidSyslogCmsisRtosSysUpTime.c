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

/* osKernelGetTickCount is 32-bit on every CMSIS-RTOS2 implementation and the
 * counter cannot say how often it has wrapped, so the phase is kept here. */
static uint32_t CmsisRtosSysUpTime_LastTicks;
static uint32_t CmsisRtosSysUpTime_Rollovers;

static uint64_t CmsisRtosSysUpTime_Extend(uint32_t nowTicks);
static uint32_t CmsisRtosSysUpTime_Hundredths(uint64_t ticks, uint32_t tickFreqHz);

uint32_t SolidSyslogCmsisRtos_GetSysUpTime(void)
{
    (void) osKernelLock();
    uint64_t ticks = CmsisRtosSysUpTime_Extend(osKernelGetTickCount());
    (void) osKernelRestoreLock(0);

    return CmsisRtosSysUpTime_Hundredths(ticks, osKernelGetTickFreq());
}

/* Missing a wrap needs 50 days of silence at 1000 Hz; every message calls in. */
static uint64_t CmsisRtosSysUpTime_Extend(uint32_t nowTicks)
{
    if (nowTicks < CmsisRtosSysUpTime_LastTicks)
    {
        CmsisRtosSysUpTime_Rollovers++;
    }
    CmsisRtosSysUpTime_LastTicks = nowTicks;

    return ((uint64_t) CmsisRtosSysUpTime_Rollovers << 32U) | (uint64_t) nowTicks;
}

static uint32_t CmsisRtosSysUpTime_Hundredths(uint64_t ticks, uint32_t tickFreqHz)
{
    /* Divide before scaling so the intermediate cannot overflow; the uint32
     * cast wraps at 2^32 hundredths, as RFC 3418 TimeTicks does. */
    uint64_t wholeSecondHundredths = (ticks / tickFreqHz) * CMSISRTOS_SYS_UP_TIME_HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths =
        ((ticks % tickFreqHz) * CMSISRTOS_SYS_UP_TIME_HUNDREDTHS_PER_SECOND) / tickFreqHz;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}
