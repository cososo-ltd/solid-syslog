/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFreeRtosSysUpTime.h"

#include "FreeRTOS.h"
#include "task.h"

#if (configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS)

enum
{
    HUNDREDTHS_PER_SECOND = 100
};

uint32_t SolidSyslogFreeRtos_GetSysUpTime(void)
{
    /* Divide before scaling so the intermediate cannot overflow. */
    uint64_t ticks = (uint64_t) xTaskGetTickCount();
    uint64_t wholeSecondHundredths = (ticks / configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND;
    uint64_t subSecondHundredths = ((ticks % configTICK_RATE_HZ) * HUNDREDTHS_PER_SECOND) / configTICK_RATE_HZ;
    return (uint32_t) (wholeSecondHundredths + subSecondHundredths);
}

#endif /* configTICK_TYPE_WIDTH_IN_BITS */
