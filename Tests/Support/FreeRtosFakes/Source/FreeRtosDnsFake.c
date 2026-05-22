// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- API shape (same-type params) is dictated by FreeRTOS-Plus-TCP

#include "FreeRtosDnsFake.h"

#include <arpa/inet.h>
#include <stddef.h>

#include "FreeRTOS_DNS.h"
#include "FreeRTOS_IP.h"

#include "SafeString.h"

static unsigned getAddrInfoCallCount = 0;
static unsigned freeAddrInfoCallCount = 0;
static bool getAddrInfoFails = false;
static char lastGetAddrInfoHostname[256];
static BaseType_t lastGetAddrInfoSocktype = 0;

/* The fake hands back a single static result struct on success; its
 * xPrivateStorage.sockaddr is the payload `ai_addr` points at, which is the
 * shape FreeRTOS-Plus-TCP actually produces (no separate malloc per address).
 * IPv4 comes from FreeRTOS_inet_addr(node), so tests that pass dotted-quad
 * hostnames get back the parsed address with no further wiring — mirrors
 * SocketFake's inet_pton trick. */
static struct freertos_addrinfo fakeResult;

void FreeRtosDnsFake_Reset(void)
{
    getAddrInfoCallCount = 0;
    freeAddrInfoCallCount = 0;
    getAddrInfoFails = false;
    lastGetAddrInfoHostname[0] = '\0';
    lastGetAddrInfoSocktype = 0;
    fakeResult = (struct freertos_addrinfo) {0};
}

void FreeRtosDnsFake_SetGetAddrInfoFails(bool fails)
{
    getAddrInfoFails = fails;
}

unsigned FreeRtosDnsFake_GetAddrInfoCallCount(void)
{
    return getAddrInfoCallCount;
}

const char* FreeRtosDnsFake_LastGetAddrInfoHostname(void)
{
    return lastGetAddrInfoHostname;
}

BaseType_t FreeRtosDnsFake_LastGetAddrInfoSocktype(void)
{
    return lastGetAddrInfoSocktype;
}

unsigned FreeRtosDnsFake_FreeAddrInfoCallCount(void)
{
    return freeAddrInfoCallCount;
}

BaseType_t FreeRTOS_getaddrinfo(
    const char* pcName,
    const char* pcService,
    const struct freertos_addrinfo* pxHints,
    struct freertos_addrinfo** ppxResult
)
{
    (void) pcService;
    getAddrInfoCallCount++;
    lastGetAddrInfoSocktype = (pxHints != NULL) ? pxHints->ai_socktype : 0;
    SafeString_Copy(lastGetAddrInfoHostname, sizeof(lastGetAddrInfoHostname), (pcName != NULL) ? pcName : "");

    BaseType_t result = -pdFREERTOS_ERRNO_ENOENT;
    if (!getAddrInfoFails)
    {
        fakeResult = (struct freertos_addrinfo) {0};
        fakeResult.ai_family = FREERTOS_AF_INET4;
        fakeResult.ai_addr = &fakeResult.xPrivateStorage.sockaddr;
        fakeResult.ai_addrlen = ipSIZE_OF_IPv4_ADDRESS;
        fakeResult.ai_addr->sin_family = FREERTOS_AF_INET;
        /* POSIX inet_addr is byte-for-byte equivalent to FreeRTOS_inet_addr
         * on the host: both return the dotted-quad parsed into a uint32_t
         * in network byte order. Using the POSIX one avoids pulling the
         * real FreeRTOS-Plus-TCP source into the host test build just to
         * provide one symbol. */
        fakeResult.ai_addr->sin_address.ulIP_IPv4 = inet_addr((pcName != NULL) ? pcName : "");
        *ppxResult = &fakeResult;
        result = 0;
    }
    return result;
}

void FreeRTOS_freeaddrinfo(struct freertos_addrinfo* pxInfo)
{
    (void) pxInfo;
    freeAddrInfoCallCount++;
}

// NOLINTEND(bugprone-easily-swappable-parameters)
