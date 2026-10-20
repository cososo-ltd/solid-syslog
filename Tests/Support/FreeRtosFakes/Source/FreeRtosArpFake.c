#include "FreeRtosArpFake.h"

#include "FreeRTOS_ARP.h"
#include "FreeRTOS.h"

#include <stddef.h>

static unsigned isIpInArpCacheCallCount = 0U;
static uint32_t lastIsIpInArpCacheArg = 0U;
static bool cacheHit = false;
static unsigned lookupsUntilHit = 0U;
static uint32_t gateway = 0U;
static bool replyPending = false;

static unsigned getCacheEntryCallCount = 0U;
static uint32_t lastGetCacheEntryArg = 0U;

static unsigned outputArpRequestCallCount = 0U;
static uint32_t lastOutputArpRequestArg = 0U;

void FreeRtosArpFake_Reset(void)
{
    isIpInArpCacheCallCount = 0;
    lastIsIpInArpCacheArg = 0;
    cacheHit = false;
    lookupsUntilHit = 0;
    gateway = 0;
    replyPending = false;

    getCacheEntryCallCount = 0;
    lastGetCacheEntryArg = 0;

    outputArpRequestCallCount = 0;
    lastOutputArpRequestArg = 0;
}

void FreeRtosArpFake_SetCacheHit(bool hit)
{
    cacheHit = hit;
}

void FreeRtosArpFake_SetLookupsUntilHit(unsigned misses)
{
    cacheHit = false;
    lookupsUntilHit = misses;
}

void FreeRtosArpFake_SetGateway(uint32_t nextHop)
{
    gateway = nextHop;
}

void FreeRtosArpFake_SetReplyPending(bool pending)
{
    replyPending = pending;
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

unsigned FreeRtosArpFake_GetCacheEntryCallCount(void)
{
    return getCacheEntryCallCount;
}

uint32_t FreeRtosArpFake_LastGetCacheEntryArg(void)
{
    return lastGetCacheEntryArg;
}

eARPLookupResult_t eARPGetCacheEntry(
    uint32_t* pulIPAddress,
    MACAddress_t* const pxMACAddress,
    struct xNetworkEndPoint** ppxEndPoint
)
{
    (void) pxMACAddress;
    *ppxEndPoint = NULL;
    ++getCacheEntryCallCount;
    lastGetCacheEntryArg = *pulIPAddress;
    if (gateway != 0U)
    {
        *pulIPAddress = gateway;
    }
    bool hit = cacheHit;
    if (lookupsUntilHit > 0U)
    {
        --lookupsUntilHit;
        hit = false;
        cacheHit = (lookupsUntilHit == 0U);
    }
    eARPLookupResult_t result = replyPending ? eCantSendPacket : eARPCacheMiss;
    return hit ? eARPCacheHit : result;
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
