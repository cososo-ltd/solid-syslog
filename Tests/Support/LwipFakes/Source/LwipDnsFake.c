#include "LwipDnsFake.h"

#include <stddef.h>

#include "LwipFakeMarshalGuard.h"
#include "lwip/dns.h"
#include "lwip/err.h"

static unsigned getHostByNameCallCount = 0;
static const char* lastHostname = NULL;
static ip_addr_t* lastAddrOut = NULL;
static err_t getHostByNameResult = ERR_OK;
static ip_addr_t resolvedIp;

static dns_found_callback storedCallback = NULL;
static void* storedCallbackArg = NULL;

void LwipDnsFake_Reset(void)
{
    getHostByNameCallCount = 0;
    lastHostname = NULL;
    lastAddrOut = NULL;
    getHostByNameResult = ERR_OK;
    ip_addr_set_zero(&resolvedIp);
    storedCallback = NULL;
    storedCallbackArg = NULL;
}

void LwipDnsFake_SetResult(int8_t err)
{
    getHostByNameResult = (err_t) err;
}

void LwipDnsFake_SetResolvedIp(const ip_addr_t* ip)
{
    resolvedIp = *ip;
}

unsigned LwipDnsFake_GetHostByNameCallCount(void)
{
    return getHostByNameCallCount;
}

const char* LwipDnsFake_LastHostname(void)
{
    return lastHostname;
}

ip_addr_t* LwipDnsFake_LastAddrOut(void)
{
    return lastAddrOut;
}

bool LwipDnsFake_HasPendingCallback(void)
{
    return storedCallback != NULL;
}

void LwipDnsFake_FireCallback(const ip_addr_t* ipaddr)
{
    dns_found_callback callback = storedCallback;
    void* arg = storedCallbackArg;
    storedCallback = NULL;
    storedCallbackArg = NULL;
    if (callback != NULL)
    {
        callback(lastHostname, ipaddr, arg);
    }
}

err_t dns_gethostbyname(const char* hostname, ip_addr_t* addr, dns_found_callback found, void* callback_arg)
{
    LWIP_REQUIRE_MARSHAL_ACTIVE();
    ++getHostByNameCallCount;
    lastHostname = hostname;
    lastAddrOut = addr;
    if (getHostByNameResult == ERR_OK)
    {
        *addr = resolvedIp;
    }
    else if (getHostByNameResult == ERR_INPROGRESS)
    {
        storedCallback = found;
        storedCallbackArg = callback_arg;
    }
    else
    {
        /* ERR_ARG / other immediate rejection - no callback stored. */
    }
    return getHostByNameResult;
}
