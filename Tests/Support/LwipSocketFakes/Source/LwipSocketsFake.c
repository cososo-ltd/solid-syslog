#include "LwipSocketsFake.h"

#include <errno.h>
#include <string.h>

#include "lwip/sockets.h"

enum
{
    FAKE_SOCKET_DESCRIPTOR = 3,
    FAKE_PAYLOAD_CAPACITY = 2048
};

static unsigned socketCallCount = 0U;
static int socketResult = FAKE_SOCKET_DESCRIPTOR;
static int lastSocketDomain = 0;
static int lastSocketType = 0;
static int lastSocketProtocol = 0;

static unsigned sendToCallCount = 0U;
static int lastSendToSocket = 0;
static char lastSendToPayload[FAKE_PAYLOAD_CAPACITY];
static size_t lastSendToSize = 0U;
static int lastSendToFlags = 0;
static struct sockaddr_in lastSendToAddress;
static socklen_t lastSendToAddressLength = 0;

static int sendToErrno = 0;

static unsigned closeCallCount = 0U;
static int lastClosedSocket = 0;

void LwipSocketsFake_Reset(void)
{
    socketCallCount = 0U;
    socketResult = FAKE_SOCKET_DESCRIPTOR;
    lastSocketDomain = 0;
    lastSocketType = 0;
    lastSocketProtocol = 0;

    sendToCallCount = 0U;
    lastSendToSocket = 0;
    (void) memset(lastSendToPayload, 0, sizeof(lastSendToPayload));
    lastSendToSize = 0U;
    lastSendToFlags = 0;
    (void) memset(&lastSendToAddress, 0, sizeof(lastSendToAddress));
    lastSendToAddressLength = 0;
    sendToErrno = 0;

    closeCallCount = 0U;
    lastClosedSocket = 0;
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

void LwipSocketsFake_SetSendToFailure(int err)
{
    sendToErrno = err;
}

unsigned LwipSocketsFake_SendToCallCount(void)
{
    return sendToCallCount;
}

int LwipSocketsFake_LastSendToSocket(void)
{
    return lastSendToSocket;
}

const void* LwipSocketsFake_LastSendToPayload(void)
{
    return lastSendToPayload;
}

size_t LwipSocketsFake_LastSendToSize(void)
{
    return lastSendToSize;
}

int LwipSocketsFake_LastSendToFlags(void)
{
    return lastSendToFlags;
}

const struct sockaddr_in* LwipSocketsFake_LastSendToAddress(void)
{
    return &lastSendToAddress;
}

socklen_t LwipSocketsFake_LastSendToAddressLength(void)
{
    return lastSendToAddressLength;
}

unsigned LwipSocketsFake_CloseCallCount(void)
{
    return closeCallCount;
}

int LwipSocketsFake_LastClosedSocket(void)
{
    return lastClosedSocket;
}

int lwip_socket(int domain, int type, int protocol)
{
    socketCallCount++;
    lastSocketDomain = domain;
    lastSocketType = type;
    lastSocketProtocol = protocol;
    return socketResult;
}

ssize_t lwip_sendto(int s, const void* dataptr, size_t size, int flags, const struct sockaddr* to, socklen_t tolen)
{
    sendToCallCount++;
    lastSendToSocket = s;
    lastSendToSize = size;
    lastSendToFlags = flags;
    lastSendToAddressLength = tolen;
    if (size <= sizeof(lastSendToPayload))
    {
        (void) memcpy(lastSendToPayload, dataptr, size);
    }
    if (to != NULL)
    {
        (void) memcpy(&lastSendToAddress, to, sizeof(lastSendToAddress));
    }

    ssize_t result = (ssize_t) size;
    if (sendToErrno != 0)
    {
        errno = sendToErrno;
        result = -1;
    }
    return result;
}

int lwip_close(int s)
{
    closeCallCount++;
    lastClosedSocket = s;
    return 0;
}
