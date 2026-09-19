#include "CmsisRtosKernelFake.h"

#include <stdint.h>

#include "cmsis_os2.h"

/* Reset leaves a frequency standing so a test that forgets to state one
 * fails its assertion rather than dividing by zero. Every test states it
 * anyway, because a tick is only a hundredth at 100 Hz. */
enum
{
    KERNEL_FAKE_DEFAULT_TICK_FREQ_HZ = 100
};

static uint32_t kernelFakeTickCount;
static uint32_t kernelFakeTickFreq = KERNEL_FAKE_DEFAULT_TICK_FREQ_HZ;

void CmsisRtosKernelFake_Reset(void)
{
    kernelFakeTickCount = 0;
    kernelFakeTickFreq = KERNEL_FAKE_DEFAULT_TICK_FREQ_HZ;
}

void CmsisRtosKernelFake_SetTickCount(uint32_t ticks)
{
    kernelFakeTickCount = ticks;
}

void CmsisRtosKernelFake_SetTickFreq(uint32_t hertz)
{
    kernelFakeTickFreq = hertz;
}

uint32_t osKernelGetTickCount(void)
{
    return kernelFakeTickCount;
}

uint32_t osKernelGetTickFreq(void)
{
    return kernelFakeTickFreq;
}
