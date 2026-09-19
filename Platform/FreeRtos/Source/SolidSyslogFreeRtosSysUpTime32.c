/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFreeRtosSysUpTime.h"

#include "FreeRTOS.h"
#include "task.h"

#if (configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS)

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

static uint64_t FreeRtosSysUpTime_Extend(uint32_t nowTicks);
static uint32_t FreeRtosSysUpTime_Hundredths(uint64_t ticks);

uint32_t SolidSyslogFreeRtos_GetSysUpTime(void)
{
    /* Extend touches shared state and runs wherever a message is formatted,
     * which may be more than one task. The scaling is pure and stays out. */
    taskENTER_CRITICAL();
    uint64_t ticks = FreeRtosSysUpTime_Extend((uint32_t) xTaskGetTickCount());
    taskEXIT_CRITICAL();

    return FreeRtosSysUpTime_Hundredths(ticks);
}

/* The counter cannot say how often it has wrapped, so the phase is kept here.
 * Missing a wrap needs 50 days of silence at 1000 Hz; every message calls in. */
static uint64_t FreeRtosSysUpTime_Extend(uint32_t nowTicks)
{
    static uint32_t lastTicks;
    static uint32_t rollovers;

    if (nowTicks < lastTicks)
    {
        rollovers++;
    }
    lastTicks = nowTicks;

    return ((uint64_t) rollovers << 32U) | (uint64_t) nowTicks;
}

static uint32_t FreeRtosSysUpTime_Hundredths(uint64_t ticks)
{
    /* Divide before scaling so the intermediate cannot overflow; the uint32
     * cast wraps at 2^32 hundredths, as RFC 3418 TimeTicks does. */
    uint64_t wholeSecondHundredths = (ticks / configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths = ((ticks % configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND) / configTICK_RATE_HZ;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int SolidSyslogFreeRtosSysUpTime32_EmptyTranslationUnit;

#endif /* configTICK_TYPE_WIDTH_IN_BITS */
