/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFreeRtosSysUpTime.h"

#include "SolidSyslogFreeRtosSysUpTimePrivate.h"

#include "FreeRTOS.h"
#include "task.h"

uint32_t SolidSyslogFreeRtos_GetSysUpTime(void)
{
    /* The phase the tick counter cannot carry. Block scope because no other
     * function needs it, and one instance per process is what uptime means:
     * SolidSyslogSysUpTimeFunction is a bare uint32_t(void) with nowhere to
     * hang an instance. */
    static struct SolidSyslogFreeRtosSysUpTimeState state;

    /* Only the read-modify-write of that state needs protecting, and it runs
     * wherever a message is formatted, which may be more than one task. The
     * section spans a compare and two stores; the scaling is pure and stays
     * outside it. */
    taskENTER_CRITICAL();
    uint64_t ticks = SolidSyslogFreeRtosSysUpTime_Extend(&state, (uint32_t) xTaskGetTickCount());
    taskEXIT_CRITICAL();

    return SolidSyslogFreeRtosSysUpTime_Hundredths(ticks, (uint32_t) configTICK_RATE_HZ);
}
