#include "FreeRtosTaskFake.h"

static unsigned   vTaskDelayCallCount = 0;
static TickType_t lastVTaskDelayTicks = 0;

void FreeRtosTaskFake_Reset(void)
{
    vTaskDelayCallCount = 0;
    lastVTaskDelayTicks = 0;
}

unsigned FreeRtosTaskFake_VTaskDelayCallCount(void)
{
    return vTaskDelayCallCount;
}

TickType_t FreeRtosTaskFake_LastVTaskDelayTicks(void)
{
    return lastVTaskDelayTicks;
}

void vTaskDelay(const TickType_t xTicksToDelay)
{
    ++vTaskDelayCallCount;
    lastVTaskDelayTicks = xTicksToDelay;
}
