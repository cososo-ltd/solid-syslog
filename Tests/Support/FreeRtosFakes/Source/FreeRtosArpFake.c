#include "FreeRtosArpFake.h"

#include "FreeRTOS_ARP.h"

static unsigned isIpInArpCacheCallCount = 0;
static uint32_t lastIsIpInArpCacheArg   = 0;
static bool     cacheHit                = false;

static unsigned outputArpRequestCallCount = 0;
static uint32_t lastOutputArpRequestArg   = 0;

void FreeRtosArpFake_Reset(void)
{
    isIpInArpCacheCallCount = 0;
    lastIsIpInArpCacheArg   = 0;
    cacheHit                = false;

    outputArpRequestCallCount = 0;
    lastOutputArpRequestArg   = 0;
}

void FreeRtosArpFake_SetCacheHit(bool hit)
{
    cacheHit = hit;
}

unsigned FreeRtosArpFake_IsIpInArpCacheCallCount(void)
{
    return isIpInArpCacheCallCount;
}

uint32_t FreeRtosArpFake_LastIsIpInArpCacheArg(void)
{
    return lastIsIpInArpCacheArg;
}

BaseType_t xIsIPInARPCache(uint32_t ulAddressToLookup)
{
    ++isIpInArpCacheCallCount;
    lastIsIpInArpCacheArg = ulAddressToLookup;
    return cacheHit ? pdTRUE : pdFALSE;
}

unsigned FreeRtosArpFake_OutputArpRequestCallCount(void)
{
    return outputArpRequestCallCount;
}

uint32_t FreeRtosArpFake_LastOutputArpRequestArg(void)
{
    return lastOutputArpRequestArg;
}

void FreeRTOS_OutputARPRequest(uint32_t ulIPAddress)
{
    ++outputArpRequestCallCount;
    lastOutputArpRequestArg = ulIPAddress;
}
