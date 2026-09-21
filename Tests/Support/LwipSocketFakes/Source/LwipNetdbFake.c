#include "LwipNetdbFake.h"

#include <stddef.h>
#include <string.h>

#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"

static unsigned getAddrInfoCallCount = 0U;
static const char* lastNodename = NULL;
static const char* lastServname = NULL;
static int lastHintsFamily = 0;
static int getAddrInfoReturn = 0;
static int resultFamily = AF_INET;
static ip4_addr_t resultIp;

static unsigned freeAddrInfoCallCount = 0U;
static const struct addrinfo* lastFreed = NULL;

/* The answer handed back by reference, standing in for the MEMP_NETDB entry the
   real lookup allocates. Static rather than allocated so a test that forgets to
   free leaks nothing; the free count is what the tests assert on instead. */
static struct addrinfo answer;
static struct sockaddr_in answerSockaddr;

void LwipNetdbFake_Reset(void)
{
    getAddrInfoCallCount = 0U;
    lastNodename = NULL;
    lastServname = NULL;
    lastHintsFamily = 0;
    getAddrInfoReturn = 0;
    resultFamily = AF_INET;
    ip4_addr_set_zero(&resultIp);

    freeAddrInfoCallCount = 0U;
    lastFreed = NULL;

    (void) memset(&answer, 0, sizeof(answer));
    (void) memset(&answerSockaddr, 0, sizeof(answerSockaddr));
}

void LwipNetdbFake_SetIpv4Result(const char* literal)
{
    (void) ip4addr_aton(literal, &resultIp);
}

void LwipNetdbFake_SetResultFamily(int family)
{
    resultFamily = family;
}

void LwipNetdbFake_SetReturn(int value)
{
    getAddrInfoReturn = value;
}

unsigned LwipNetdbFake_GetAddrInfoCallCount(void)
{
    return getAddrInfoCallCount;
}

const char* LwipNetdbFake_LastNodename(void)
{
    return lastNodename;
}

const char* LwipNetdbFake_LastServname(void)
{
    return lastServname;
}

int LwipNetdbFake_LastHintsFamily(void)
{
    return lastHintsFamily;
}

unsigned LwipNetdbFake_FreeAddrInfoCallCount(void)
{
    return freeAddrInfoCallCount;
}

const struct addrinfo* LwipNetdbFake_LastFreed(void)
{
    return lastFreed;
}

int lwip_getaddrinfo(
    const char* nodename,
    const char* servname,
    const struct addrinfo* hints,
    struct addrinfo** res
)
{
    getAddrInfoCallCount++;
    lastNodename = nodename;
    lastServname = servname;
    lastHintsFamily = (hints != NULL) ? hints->ai_family : 0;

    int result = getAddrInfoReturn;
    if (result == 0)
    {
        answerSockaddr.sin_family = (u8_t) resultFamily;
        answerSockaddr.sin_addr.s_addr = resultIp.addr;
        answer.ai_family = resultFamily;
        answer.ai_addr = (struct sockaddr*) &answerSockaddr;
        answer.ai_addrlen = sizeof(answerSockaddr);
        *res = &answer;
    }
    else
    {
        *res = NULL;
    }
    return result;
}

void lwip_freeaddrinfo(struct addrinfo* ai)
{
    freeAddrInfoCallCount++;
    lastFreed = ai;
}
