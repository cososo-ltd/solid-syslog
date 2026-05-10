#include "FreeRtosTaskFake.h"

static unsigned   vTaskDelayCallCount = 0;
static TickType_t lastVTaskDelayTicks = 0;
static TickType_t tickCount           = 0;

void FreeRtosTaskFake_Reset(void)
{
    vTaskDelayCallCount = 0;
    lastVTaskDelayTicks = 0;
    tickCount           = 0;
}

unsigned FreeRtosTaskFake_VTaskDelayCallCount(void)
{
    return vTaskDelayCallCount;
}

TickType_t FreeRtosTaskFake_LastVTaskDelayTicks(void)
{
    return lastVTaskDelayTicks;
}

void FreeRtosTaskFake_SetTickCount(TickType_t ticks)
{
    tickCount = ticks;
}

void vTaskDelay(const TickType_t xTicksToDelay)
{
    ++vTaskDelayCallCount;
    lastVTaskDelayTicks = xTicksToDelay;
}

TickType_t xTaskGetTickCount(void)
{
    return tickCount;
}
