#include "LwipSocketsFake.h"

#include "lwip/sockets.h"

enum
{
    FAKE_SOCKET_DESCRIPTOR = 3
};

static unsigned socketCallCount = 0U;
static int socketResult = FAKE_SOCKET_DESCRIPTOR;
static int lastSocketDomain = 0;
static int lastSocketType = 0;
static int lastSocketProtocol = 0;

void LwipSocketsFake_Reset(void)
{
    socketCallCount = 0U;
    socketResult = FAKE_SOCKET_DESCRIPTOR;
    lastSocketDomain = 0;
    lastSocketType = 0;
    lastSocketProtocol = 0;
}

void LwipSocketsFake_SetSocketResult(int result)
{
    socketResult = result;
}

unsigned LwipSocketsFake_SocketCallCount(void)
{
    return socketCallCount;
}

int LwipSocketsFake_LastSocketDomain(void)
{
    return lastSocketDomain;
}

int LwipSocketsFake_LastSocketType(void)
{
    return lastSocketType;
}

int LwipSocketsFake_LastSocketProtocol(void)
{
    return lastSocketProtocol;
}

int lwip_socket(int domain, int type, int protocol)
{
    socketCallCount++;
    lastSocketDomain = domain;
    lastSocketType = type;
    lastSocketProtocol = protocol;
    return socketResult;
}
