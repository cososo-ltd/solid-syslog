/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFreeRtosSysUpTime.h"

#include "SolidSyslogFreeRtosSysUpTimePrivate.h"

#include "FreeRTOS.h"
#include "task.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

/* The phase the tick counter cannot carry. File-scope because the callback
 * takes no context: SolidSyslogSysUpTimeFunction is a bare uint32_t(void),
 * and one instance per process is what uptime means anyway. */
static struct SolidSyslogFreeRtosSysUpTimeState SysUpTime_State;

uint32_t SolidSyslogFreeRtos_GetSysUpTime(void)
{
    /* Read and update under a critical section: the read-modify-write of the
     * rollover state is not atomic, and this runs wherever a message is
     * formatted, which may be more than one task. The section spans a compare
     * and two stores, so the interrupt latency it adds is bounded and tiny. */
    uint32_t hundredths = 0U;
    taskENTER_CRITICAL();
    hundredths = SolidSyslogFreeRtosSysUpTime_Advance(
        &SysUpTime_State,
        (uint32_t) xTaskGetTickCount(),
        (uint32_t) configTICK_RATE_HZ
    );
    taskEXIT_CRITICAL();
    return hundredths;
}

uint32_t SolidSyslogFreeRtosSysUpTime_Advance(
    struct SolidSyslogFreeRtosSysUpTimeState* state,
    uint32_t nowTicks,
    uint32_t tickRateHz
)
{
    /* A count below the last one seen can only mean the counter wrapped. */
    if (nowTicks < state->LastTicks)
    {
        state->Rollovers++;
    }
    state->LastTicks = nowTicks;

    /* Divide the tick count down before scaling by 100 so the intermediate
     * cannot overflow; the whole/remainder split is exact floor division, and
     * the uint32 cast wraps at 2^32 hundredths per RFC 3418 TimeTicks. */
    uint64_t ticks = ((uint64_t) state->Rollovers << 32U) | (uint64_t) nowTicks;
    uint64_t wholeSecondHundredths = (ticks / tickRateHz) * HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths = ((ticks % tickRateHz) * HUNDREDTHS_PER_SECOND) / tickRateHz;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}
