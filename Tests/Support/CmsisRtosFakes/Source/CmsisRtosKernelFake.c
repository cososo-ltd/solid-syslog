#include "CmsisRtosKernelFake.h"

#include <stdint.h>

#include "cmsis_os2.h"

static uint32_t kernelFakeTickCount;

void CmsisRtosKernelFake_Reset(void)
{
    kernelFakeTickCount = 0;
}

void CmsisRtosKernelFake_SetTickCount(uint32_t ticks)
{
    kernelFakeTickCount = ticks;
}

uint32_t osKernelGetTickCount(void)
{
    return kernelFakeTickCount;
}
