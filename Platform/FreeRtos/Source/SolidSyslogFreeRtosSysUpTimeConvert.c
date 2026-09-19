/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The uptime conversion, kept apart from the tick source it serves. Nothing
 * here includes a FreeRTOS header: the rollover accounting and the scaling
 * are arithmetic, so they are reachable from a test at tick rates the build
 * is not compiled at, and the entry point beside them keeps the coupling. */
#include "SolidSyslogFreeRtosSysUpTimePrivate.h"

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

uint64_t SolidSyslogFreeRtosSysUpTime_Extend(struct SolidSyslogFreeRtosSysUpTimeState* self, uint64_t nowTicks)
{
    /* A count below the last one seen can only mean the counter wrapped. */
    if (nowTicks < self->LastTicks)
    {
        self->Rollovers++;
    }
    self->LastTicks = nowTicks;

    /* Each wrap is a 32-bit counter's worth of ticks. A 64-bit TickType_t
     * arrives whole and never decreases, so it takes this path with no
     * rollovers and is returned unchanged. A 16-bit one is not extended -
     * it was not before either, and #755 asks only about the 32-bit case. */
    return (self->Rollovers << 32U) + nowTicks;
}

uint32_t SolidSyslogFreeRtosSysUpTime_Hundredths(uint64_t ticks, uint32_t tickRateHz)
{
    /* Divide the tick count down before scaling by 100 so the intermediate
     * cannot overflow; the whole/remainder split is exact floor division, and
     * the uint32 cast wraps at 2^32 hundredths per RFC 3418 TimeTicks. */
    uint64_t wholeSecondHundredths = (ticks / tickRateHz) * HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths = ((ticks % tickRateHz) * HUNDREDTHS_PER_SECOND) / tickRateHz;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}
