#include "LwipDnsFake.h"

#include <stddef.h>

#include "LwipFakeMarshalGuard.h"
#include "lwip/dns.h"
#include "lwip/err.h"

static unsigned getHostByNameCallCount = 0U;
static const char* lastHostname = NULL;
static ip_addr_t* lastAddrOut = NULL;
static uint8_t lastAddrType = 0U;
static err_t getHostByNameResult = ERR_OK;
static ip_addr_t resolvedIp;

static dns_found_callback storedCallback = NULL;
static void* storedCallbackArg = NULL;

void LwipDnsFake_Reset(void)
{
    getHostByNameCallCount = 0;
    lastHostname = NULL;
    lastAddrOut = NULL;
    lastAddrType = 0U;
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

uint8_t LwipDnsFake_LastAddrType(void)
{
    return lastAddrType;
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

err_t dns_gethostbyname_addrtype(
    const char* hostname,
    ip_addr_t* addr,
    dns_found_callback found,
    void* callback_arg,
    u8_t dns_addrtype
)
{
    LWIP_REQUIRE_MARSHAL_ACTIVE();
    ++getHostByNameCallCount;
    lastHostname = hostname;
    lastAddrOut = addr;
    lastAddrType = dns_addrtype;
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
