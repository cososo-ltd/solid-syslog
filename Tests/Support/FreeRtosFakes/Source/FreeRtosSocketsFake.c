// NOLINTBEGIN(performance-no-int-to-ptr,bugprone-easily-swappable-parameters) -- API shape (sentinel cast + same-type params) is dictated by FreeRTOS-Plus-TCP

#include "FreeRtosSocketsFake.h"

static unsigned   socketCallCount    = 0;
static BaseType_t lastSocketDomain   = 0;
static BaseType_t lastSocketType     = 0;
static BaseType_t lastSocketProto    = 0;
static Socket_t   lastSocketReturned = NULL;
static bool       socketFails        = false;

static unsigned                        sendtoCallCount             = 0;
static Socket_t                        lastSendtoSocket            = NULL;
static const void*                     lastSendtoBuffer            = NULL;
static size_t                          lastSendtoLength            = 0;
static BaseType_t                      lastSendtoFlags             = 0;
static const struct freertos_sockaddr* lastSendtoDestination       = NULL;
static socklen_t                       lastSendtoDestinationLength = 0;
static bool                            sendtoFails                 = false;

static unsigned                        connectCallCount         = 0;
static Socket_t                        lastConnectSocket        = NULL;
static const struct freertos_sockaddr* lastConnectAddress       = NULL;
static socklen_t                       lastConnectAddressLength = 0;
static bool                            connectFails             = false;

static unsigned    sendCallCount   = 0;
static Socket_t    lastSendSocket  = NULL;
static const void* lastSendBuffer  = NULL;
static size_t      lastSendLength  = 0;
static BaseType_t  lastSendFlags   = 0;
static bool        sendFails       = false;
static bool        sendReturnSet   = false;
static BaseType_t  sendReturnValue = 0;

static unsigned   recvCallCount   = 0;
static Socket_t   lastRecvSocket  = NULL;
static void*      lastRecvBuffer  = NULL;
static size_t     lastRecvLength  = 0;
static BaseType_t lastRecvFlags   = 0;
static bool       recvFails       = false;
static bool       recvReturnSet   = false;
static BaseType_t recvReturnValue = 0;

static TickType_t lastSndTimeoSet            = 0;
static TickType_t lastRcvTimeoSet            = 0;
static unsigned   rcvTimeoSetCallCount       = 0;
static Socket_t   lastSetsockoptSocket       = NULL;
static int32_t    lastSetsockoptLevel        = 0;
static int32_t    lastSetsockoptOptionName   = 0;
static size_t     lastSetsockoptOptionLength = 0;
static TickType_t sndTimeoAtConnect          = 0;
static TickType_t rcvTimeoAtConnect          = 0;

static unsigned closesocketCallCount  = 0;
static Socket_t lastClosesocketSocket = NULL;

/* Sentinel used as a "valid" Socket_t return — non-NULL and not
 * FREERTOS_INVALID_SOCKET. Adapters only inspect Open's return for
 * the invalid sentinel; any other non-zero pointer is treated as success. */
static int fakeSocketHandleAnchor = 0;
#define FAKE_VALID_SOCKET ((Socket_t) & fakeSocketHandleAnchor)

void FreeRtosSocketsFake_Reset(void)
{
    socketCallCount    = 0;
    lastSocketDomain   = 0;
    lastSocketType     = 0;
    lastSocketProto    = 0;
    lastSocketReturned = NULL;
    socketFails        = false;

    sendtoCallCount             = 0;
    lastSendtoSocket            = NULL;
    lastSendtoBuffer            = NULL;
    lastSendtoLength            = 0;
    lastSendtoFlags             = 0;
    lastSendtoDestination       = NULL;
    lastSendtoDestinationLength = 0;
    sendtoFails                 = false;

    connectCallCount         = 0;
    lastConnectSocket        = NULL;
    lastConnectAddress       = NULL;
    lastConnectAddressLength = 0;
    connectFails             = false;

    sendCallCount   = 0;
    lastSendSocket  = NULL;
    lastSendBuffer  = NULL;
    lastSendLength  = 0;
    lastSendFlags   = 0;
    sendFails       = false;
    sendReturnSet   = false;
    sendReturnValue = 0;

    recvCallCount   = 0;
    lastRecvSocket  = NULL;
    lastRecvBuffer  = NULL;
    lastRecvLength  = 0;
    lastRecvFlags   = 0;
    recvFails       = false;
    recvReturnSet   = false;
    recvReturnValue = 0;

    lastSndTimeoSet            = 0;
    lastRcvTimeoSet            = 0;
    rcvTimeoSetCallCount       = 0;
    lastSetsockoptSocket       = NULL;
    lastSetsockoptLevel        = 0;
    lastSetsockoptOptionName   = 0;
    lastSetsockoptOptionLength = 0;
    sndTimeoAtConnect          = 0;
    rcvTimeoAtConnect          = 0;

    closesocketCallCount  = 0;
    lastClosesocketSocket = NULL;
}

void FreeRtosSocketsFake_SetSocketFails(bool fails)
{
    socketFails = fails;
}

void FreeRtosSocketsFake_SetSendtoFails(bool fails)
{
    sendtoFails = fails;
}

void FreeRtosSocketsFake_SetConnectFails(bool fails)
{
    connectFails = fails;
}

void FreeRtosSocketsFake_SetSendFails(bool fails)
{
    sendFails = fails;
}

void FreeRtosSocketsFake_SetSendReturn(BaseType_t value)
{
    sendReturnSet   = true;
    sendReturnValue = value;
}

void FreeRtosSocketsFake_SetRecvFails(bool fails)
{
    recvFails = fails;
}

void FreeRtosSocketsFake_SetRecvReturn(BaseType_t value)
{
    recvReturnSet   = true;
    recvReturnValue = value;
}

unsigned FreeRtosSocketsFake_SocketCallCount(void)
{
    return socketCallCount;
}

BaseType_t FreeRtosSocketsFake_LastSocketDomain(void)
{
    return lastSocketDomain;
}

BaseType_t FreeRtosSocketsFake_LastSocketType(void)
{
    return lastSocketType;
}

BaseType_t FreeRtosSocketsFake_LastSocketProtocol(void)
{
    return lastSocketProto;
}

Socket_t FreeRtosSocketsFake_LastSocketReturned(void)
{
    return lastSocketReturned;
}

unsigned FreeRtosSocketsFake_SendtoCallCount(void)
{
    return sendtoCallCount;
}

Socket_t FreeRtosSocketsFake_LastSendtoSocket(void)
{
    return lastSendtoSocket;
}

const void* FreeRtosSocketsFake_LastSendtoBuffer(void)
{
    return lastSendtoBuffer;
}

size_t FreeRtosSocketsFake_LastSendtoLength(void)
{
    return lastSendtoLength;
}

BaseType_t FreeRtosSocketsFake_LastSendtoFlags(void)
{
    return lastSendtoFlags;
}

const struct freertos_sockaddr* FreeRtosSocketsFake_LastSendtoDestination(void)
{
    return lastSendtoDestination;
}

socklen_t FreeRtosSocketsFake_LastSendtoDestinationLength(void)
{
    return lastSendtoDestinationLength;
}

Socket_t FreeRTOS_socket(BaseType_t xDomain, BaseType_t xType, BaseType_t xProtocol)
{
    ++socketCallCount;
    lastSocketDomain   = xDomain;
    lastSocketType     = xType;
    lastSocketProto    = xProtocol;
    lastSocketReturned = socketFails ? FREERTOS_INVALID_SOCKET : FAKE_VALID_SOCKET;
    return lastSocketReturned;
}

int32_t FreeRTOS_sendto(Socket_t xSocket, const void* pvBuffer, size_t uxTotalDataLength, BaseType_t xFlags,
                        const struct freertos_sockaddr* pxDestinationAddress, socklen_t xDestinationAddressLength)
{
    ++sendtoCallCount;
    lastSendtoSocket            = xSocket;
    lastSendtoBuffer            = pvBuffer;
    lastSendtoLength            = uxTotalDataLength;
    lastSendtoFlags             = xFlags;
    lastSendtoDestination       = pxDestinationAddress;
    lastSendtoDestinationLength = xDestinationAddressLength;
    return sendtoFails ? -pdFREERTOS_ERRNO_ENOBUFS : (int32_t) uxTotalDataLength;
}

TickType_t FreeRtosSocketsFake_SndTimeoAtConnect(void)
{
    return sndTimeoAtConnect;
}

TickType_t FreeRtosSocketsFake_RcvTimeoAtConnect(void)
{
    return rcvTimeoAtConnect;
}

TickType_t FreeRtosSocketsFake_LastSndTimeoSet(void)
{
    return lastSndTimeoSet;
}

TickType_t FreeRtosSocketsFake_LastRcvTimeoSet(void)
{
    return lastRcvTimeoSet;
}

unsigned FreeRtosSocketsFake_RcvTimeoSetCallCount(void)
{
    return rcvTimeoSetCallCount;
}

Socket_t FreeRtosSocketsFake_LastSetsockoptSocket(void)
{
    return lastSetsockoptSocket;
}

int32_t FreeRtosSocketsFake_LastSetsockoptLevel(void)
{
    return lastSetsockoptLevel;
}

int32_t FreeRtosSocketsFake_LastSetsockoptOptionName(void)
{
    return lastSetsockoptOptionName;
}

size_t FreeRtosSocketsFake_LastSetsockoptOptionLength(void)
{
    return lastSetsockoptOptionLength;
}

BaseType_t FreeRTOS_setsockopt(Socket_t xSocket, int32_t lLevel, int32_t lOptionName, const void* pvOptionValue, size_t uxOptionLength)
{
    lastSetsockoptSocket       = xSocket;
    lastSetsockoptLevel        = lLevel;
    lastSetsockoptOptionName   = lOptionName;
    lastSetsockoptOptionLength = uxOptionLength;
    if (lOptionName == FREERTOS_SO_SNDTIMEO)
    {
        lastSndTimeoSet = *(const TickType_t*) pvOptionValue;
    }
    else if (lOptionName == FREERTOS_SO_RCVTIMEO)
    {
        lastRcvTimeoSet = *(const TickType_t*) pvOptionValue;
        ++rcvTimeoSetCallCount;
    }
    return 0;
}

BaseType_t FreeRTOS_connect(Socket_t xClientSocket, const struct freertos_sockaddr* pxAddress, socklen_t xAddressLength)
{
    ++connectCallCount;
    lastConnectSocket        = xClientSocket;
    lastConnectAddress       = pxAddress;
    lastConnectAddressLength = xAddressLength;
    sndTimeoAtConnect        = lastSndTimeoSet;
    rcvTimeoAtConnect        = lastRcvTimeoSet;
    return connectFails ? -pdFREERTOS_ERRNO_ENOTCONN : 0;
}

unsigned FreeRtosSocketsFake_ConnectCallCount(void)
{
    return connectCallCount;
}

Socket_t FreeRtosSocketsFake_LastConnectSocket(void)
{
    return lastConnectSocket;
}

const struct freertos_sockaddr* FreeRtosSocketsFake_LastConnectAddress(void)
{
    return lastConnectAddress;
}

socklen_t FreeRtosSocketsFake_LastConnectAddressLength(void)
{
    return lastConnectAddressLength;
}

BaseType_t FreeRTOS_send(Socket_t xSocket, const void* pvBuffer, size_t uxDataLength, BaseType_t xFlags)
{
    ++sendCallCount;
    lastSendSocket = xSocket;
    lastSendBuffer = pvBuffer;
    lastSendLength = uxDataLength;
    lastSendFlags  = xFlags;
    if (sendFails)
    {
        return -pdFREERTOS_ERRNO_ENOTCONN;
    }
    if (sendReturnSet)
    {
        return sendReturnValue;
    }
    return (BaseType_t) uxDataLength;
}

unsigned FreeRtosSocketsFake_SendCallCount(void)
{
    return sendCallCount;
}

Socket_t FreeRtosSocketsFake_LastSendSocket(void)
{
    return lastSendSocket;
}

const void* FreeRtosSocketsFake_LastSendBuffer(void)
{
    return lastSendBuffer;
}

size_t FreeRtosSocketsFake_LastSendLength(void)
{
    return lastSendLength;
}

BaseType_t FreeRtosSocketsFake_LastSendFlags(void)
{
    return lastSendFlags;
}

BaseType_t FreeRTOS_recv(Socket_t xSocket, void* pvBuffer, size_t uxBufferLength, BaseType_t xFlags)
{
    ++recvCallCount;
    lastRecvSocket = xSocket;
    lastRecvBuffer = pvBuffer;
    lastRecvLength = uxBufferLength;
    lastRecvFlags  = xFlags;
    if (recvFails)
    {
        return -pdFREERTOS_ERRNO_ENOTCONN;
    }
    if (recvReturnSet)
    {
        return recvReturnValue;
    }
    return 0;
}

unsigned FreeRtosSocketsFake_RecvCallCount(void)
{
    return recvCallCount;
}

Socket_t FreeRtosSocketsFake_LastRecvSocket(void)
{
    return lastRecvSocket;
}

void* FreeRtosSocketsFake_LastRecvBuffer(void)
{
    return lastRecvBuffer;
}

size_t FreeRtosSocketsFake_LastRecvLength(void)
{
    return lastRecvLength;
}

BaseType_t FreeRtosSocketsFake_LastRecvFlags(void)
{
    return lastRecvFlags;
}

unsigned FreeRtosSocketsFake_ClosesocketCallCount(void)
{
    return closesocketCallCount;
}

Socket_t FreeRtosSocketsFake_LastClosesocketSocket(void)
{
    return lastClosesocketSocket;
}

BaseType_t FreeRTOS_closesocket(Socket_t xSocket)
{
    ++closesocketCallCount;
    lastClosesocketSocket = xSocket;
    return pdPASS;
}

// NOLINTEND(performance-no-int-to-ptr,bugprone-easily-swappable-parameters)
