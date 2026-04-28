#include "WinsockFake.h"
#include "SafeString.h"
#include "SolidSyslog.h"
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

enum
{
    WINSOCKFAKE_MAX_BUFFER_SIZE      = SOLIDSYSLOG_MAX_MESSAGE_SIZE,
    WINSOCKFAKE_MAX_HOSTNAME_SIZE    = 256,
    WINSOCKFAKE_MAX_SEND_CALLS       = 8,
    WINSOCKFAKE_MAX_SETSOCKOPT_CALLS = 8
};

static bool               sendtoFails;
static bool               nextSendtoShouldFailWithLastError;
static int                nextSendtoLastError;
static int                sendtoCallCount;
static char               lastBufCopy[WINSOCKFAKE_MAX_BUFFER_SIZE];
static size_t             lastLen;
static int                lastFlags;
static struct sockaddr_in lastAddr;
static int                lastAddrLen;
static SOCKET             lastSendtoFd;

static int  ipMtuValue;
static bool ipMtuLookupFails;
static int  getSockOptCallCount;

static bool   socketFails;
static int    socketCallCount;
static SOCKET socketFd;
static int    lastSocketDomain;
static int    lastSocketType;

static bool               connectFails;
static int                connectCallCount;
static SOCKET             lastConnectFd;
static struct sockaddr_in lastConnectAddr;
static char               lastConnectAddrString[INET_ADDRSTRLEN];

static bool   sendFails;
static bool   sendReturnOverride;
static int    sendReturnValue;
static int    sendCallCount;
static char   sendBufCopy[WINSOCKFAKE_MAX_SEND_CALLS][WINSOCKFAKE_MAX_BUFFER_SIZE];
static size_t sendLenCopy[WINSOCKFAKE_MAX_SEND_CALLS];
static int    sendFlagsCopy[WINSOCKFAKE_MAX_SEND_CALLS];
static SOCKET lastSendFd;

static int         recvCallCount;
static int         recvReturn;
static SOCKET      lastRecvFd;
static const void* lastRecvBuf;
static size_t      lastRecvLen;
static int         lastRecvFlags;

static int setSockOptCallCount;
static int lastSetSockOptLevel;
static int lastSetSockOptOptname;
static int setSockOptLevels[WINSOCKFAKE_MAX_SETSOCKOPT_CALLS];
static int setSockOptOptnames[WINSOCKFAKE_MAX_SETSOCKOPT_CALLS];

static int    closeCallCount;
static SOCKET lastClosedFd;

static char lastAddrString[INET_ADDRSTRLEN];

static bool               getAddrInfoFails;
static int                getAddrInfoCallCount;
static char               lastGetAddrInfoHostname[WINSOCKFAKE_MAX_HOSTNAME_SIZE];
static int                lastGetAddrInfoSocktype;
static struct sockaddr_in fakeResolvedAddr;
static struct addrinfo    fakeAddrInfo;
static int                freeAddrInfoCallCount;

void WinsockFake_Reset(void)
{
    WSASetLastError(0);
    sendtoFails                       = false;
    nextSendtoShouldFailWithLastError = false;
    nextSendtoLastError               = 0;
    sendtoCallCount                   = 0;
    lastBufCopy[0]                    = '\0';
    lastLen                           = 0;
    lastFlags                         = 0;
    lastAddr                          = (struct sockaddr_in) {0};
    lastAddrLen                       = 0;
    lastSendtoFd                      = INVALID_SOCKET;

    socketFails      = false;
    socketCallCount  = 0;
    socketFd         = INVALID_SOCKET;
    lastSocketDomain = 0;
    lastSocketType   = 0;

    connectFails             = false;
    connectCallCount         = 0;
    lastConnectFd            = INVALID_SOCKET;
    lastConnectAddr          = (struct sockaddr_in) {0};
    lastConnectAddrString[0] = '\0';

    sendFails          = false;
    sendReturnOverride = false;
    sendReturnValue    = 0;
    sendCallCount      = 0;
    for (int i = 0; i < WINSOCKFAKE_MAX_SEND_CALLS; i++)
    {
        sendBufCopy[i][0] = '\0';
        sendLenCopy[i]    = 0;
        sendFlagsCopy[i]  = 0;
    }
    lastSendFd = INVALID_SOCKET;

    recvCallCount = 0;
    recvReturn    = 0;
    lastRecvFd    = INVALID_SOCKET;
    lastRecvBuf   = NULL;
    lastRecvLen   = 0;
    lastRecvFlags = 0;

    setSockOptCallCount   = 0;
    lastSetSockOptLevel   = 0;
    lastSetSockOptOptname = 0;
    for (int i = 0; i < WINSOCKFAKE_MAX_SETSOCKOPT_CALLS; i++)
    {
        setSockOptLevels[i]   = 0;
        setSockOptOptnames[i] = 0;
    }
    getSockOptCallCount = 0;
    ipMtuValue          = 0;
    ipMtuLookupFails    = false;

    closeCallCount = 0;
    lastClosedFd   = INVALID_SOCKET;

    lastAddrString[0] = '\0';

    getAddrInfoFails            = false;
    getAddrInfoCallCount        = 0;
    lastGetAddrInfoHostname[0]  = '\0';
    lastGetAddrInfoSocktype     = 0;
    freeAddrInfoCallCount       = 0;
    fakeResolvedAddr            = (struct sockaddr_in) {0};
    fakeResolvedAddr.sin_family = AF_INET;
    fakeAddrInfo                = (struct addrinfo) {0};
    fakeAddrInfo.ai_family      = AF_INET;
    fakeAddrInfo.ai_addr        = (struct sockaddr*) &fakeResolvedAddr;
    fakeAddrInfo.ai_addrlen     = sizeof(fakeResolvedAddr);
}

/* socket configuration */

void WinsockFake_SetSocketFails(bool fails)
{
    socketFails = fails;
}

/* socket accessors */

int WinsockFake_SocketCallCount(void)
{
    return socketCallCount;
}

SOCKET WinsockFake_SocketFd(void)
{
    return socketFd;
}

int WinsockFake_SocketDomain(void)
{
    return lastSocketDomain;
}

int WinsockFake_SocketType(void)
{
    return lastSocketType;
}

/* sendto configuration */

void WinsockFake_SetSendtoFails(bool fails)
{
    sendtoFails = fails;
}

void WinsockFake_FailNextSendtoWithLastError(int wsaError)
{
    nextSendtoShouldFailWithLastError = true;
    nextSendtoLastError               = wsaError;
}

void WinsockFake_SetIpMtu(int mtu)
{
    ipMtuValue = mtu;
}

void WinsockFake_SetIpMtuLookupFails(bool fails)
{
    ipMtuLookupFails = fails;
}

int WinsockFake_GetSockOptCallCount(void)
{
    return getSockOptCallCount;
}

/* sendto accessors */

int WinsockFake_SendtoCallCount(void)
{
    return sendtoCallCount;
}

const char* WinsockFake_LastBufAsString(void)
{
    return lastBufCopy;
}

size_t WinsockFake_LastLen(void)
{
    return lastLen;
}

int WinsockFake_LastFlags(void)
{
    return lastFlags;
}

int WinsockFake_LastAddrFamily(void)
{
    return lastAddr.sin_family;
}

int WinsockFake_LastPort(void)
{
    return ntohs(lastAddr.sin_port);
}

const char* WinsockFake_LastAddrAsString(void)
{
    inet_ntop(AF_INET, &lastAddr.sin_addr, lastAddrString, sizeof(lastAddrString));
    return lastAddrString;
}

int WinsockFake_LastAddrLen(void)
{
    return lastAddrLen;
}

SOCKET WinsockFake_LastSendtoFd(void)
{
    return lastSendtoFd;
}

/* connect configuration */

void WinsockFake_SetConnectFails(bool fails)
{
    connectFails = fails;
}

/* connect accessors */

int WinsockFake_ConnectCallCount(void)
{
    return connectCallCount;
}

SOCKET WinsockFake_LastConnectFd(void)
{
    return lastConnectFd;
}

int WinsockFake_LastConnectPort(void)
{
    return ntohs(lastConnectAddr.sin_port);
}

const char* WinsockFake_LastConnectAddrAsString(void)
{
    inet_ntop(AF_INET, &lastConnectAddr.sin_addr, lastConnectAddrString, sizeof(lastConnectAddrString));
    return lastConnectAddrString;
}

/* send configuration */

void WinsockFake_SetSendFails(bool fails)
{
    sendFails = fails;
}

void WinsockFake_SetSendReturn(int value)
{
    sendReturnOverride = true;
    sendReturnValue    = value;
}

/* send accessors */

int WinsockFake_SendCallCount(void)
{
    return sendCallCount;
}

const char* WinsockFake_SendBufAsString(int callIndex)
{
    if (callIndex < 0 || callIndex >= WINSOCKFAKE_MAX_SEND_CALLS)
    {
        return "";
    }
    return sendBufCopy[callIndex];
}

size_t WinsockFake_SendLen(int callIndex)
{
    if (callIndex < 0 || callIndex >= WINSOCKFAKE_MAX_SEND_CALLS)
    {
        return 0;
    }
    return sendLenCopy[callIndex];
}

int WinsockFake_SendFlags(int callIndex)
{
    if (callIndex < 0 || callIndex >= WINSOCKFAKE_MAX_SEND_CALLS)
    {
        return 0;
    }
    return sendFlagsCopy[callIndex];
}

SOCKET WinsockFake_LastSendFd(void)
{
    return lastSendFd;
}

/* recv configuration */

void WinsockFake_SetRecvReturn(int value)
{
    recvReturn = value;
}

/* recv accessors */

int WinsockFake_RecvCallCount(void)
{
    return recvCallCount;
}

SOCKET WinsockFake_LastRecvFd(void)
{
    return lastRecvFd;
}

const void* WinsockFake_LastRecvBuf(void)
{
    return lastRecvBuf;
}

size_t WinsockFake_LastRecvLen(void)
{
    return lastRecvLen;
}

int WinsockFake_LastRecvFlags(void)
{
    return lastRecvFlags;
}

/* setsockopt accessors */

int WinsockFake_SetSockOptCallCount(void)
{
    return setSockOptCallCount;
}

int WinsockFake_LastSetSockOptLevel(void)
{
    return lastSetSockOptLevel;
}

int WinsockFake_LastSetSockOptOptname(void)
{
    return lastSetSockOptOptname;
}

bool WinsockFake_HasSetSockOpt(int level, int optname)
{
    int recorded = setSockOptCallCount < WINSOCKFAKE_MAX_SETSOCKOPT_CALLS ? setSockOptCallCount : WINSOCKFAKE_MAX_SETSOCKOPT_CALLS;
    for (int i = 0; i < recorded; i++)
    {
        if (setSockOptLevels[i] == level && setSockOptOptnames[i] == optname)
        {
            return true;
        }
    }
    return false;
}

/* closesocket accessors */

int WinsockFake_CloseCallCount(void)
{
    return closeCallCount;
}

SOCKET WinsockFake_LastClosedFd(void)
{
    return lastClosedFd;
}

/* getaddrinfo configuration */

void WinsockFake_SetGetAddrInfoFails(bool fails)
{
    getAddrInfoFails = fails;
}

/* getaddrinfo accessors */

int WinsockFake_GetAddrInfoCallCount(void)
{
    return getAddrInfoCallCount;
}

const char* WinsockFake_LastGetAddrInfoHostname(void)
{
    return lastGetAddrInfoHostname;
}

int WinsockFake_LastGetAddrInfoSocktype(void)
{
    return lastGetAddrInfoSocktype;
}

/* freeaddrinfo accessors */

int WinsockFake_FreeAddrInfoCallCount(void)
{
    return freeAddrInfoCallCount;
}

/* Winsock fake functions (UT_PTR_SET injection targets) */

SOCKET WSAAPI WinsockFake_socket(int af, int type, int protocol)
{
    (void) protocol;
    socketCallCount++;
    lastSocketDomain = af;
    lastSocketType   = type;
    if (socketFails)
    {
        socketFd = INVALID_SOCKET;
    }
    else
    {
        socketFd = (SOCKET) socketCallCount; /* deterministic fake handle */
    }
    return socketFd;
}

int WSAAPI WinsockFake_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    sendtoCallCount++;
    lastSendtoFd = s;
    lastFlags    = flags;
    lastAddrLen  = tolen;
    if (buf != NULL && len >= 0)
    {
        size_t copySize = (size_t) len < sizeof(lastBufCopy) - 1 ? (size_t) len : sizeof(lastBufCopy) - 1;
        memcpy(lastBufCopy, buf, copySize);
        lastBufCopy[copySize] = '\0';
        lastLen               = (size_t) len;
    }
    else
    {
        lastBufCopy[0] = '\0';
        lastLen        = 0;
    }
    if (to != NULL && tolen >= (int) sizeof(struct sockaddr_in))
    {
        lastAddr = *(const struct sockaddr_in*) to;
    }
    if (nextSendtoShouldFailWithLastError)
    {
        WSASetLastError(nextSendtoLastError);
        nextSendtoShouldFailWithLastError = false;
        return SOCKET_ERROR;
    }
    return sendtoFails ? SOCKET_ERROR : len;
}

int WSAAPI WinsockFake_connect(SOCKET s, const struct sockaddr* name, int namelen)
{
    (void) namelen;
    connectCallCount++;
    lastConnectFd = s;
    if (name != NULL && namelen >= (int) sizeof(struct sockaddr_in))
    {
        lastConnectAddr = *(const struct sockaddr_in*) name;
    }
    return connectFails ? SOCKET_ERROR : 0;
}

int WSAAPI WinsockFake_send(SOCKET s, const char* buf, int len, int flags)
{
    lastSendFd = s;
    if (sendCallCount < WINSOCKFAKE_MAX_SEND_CALLS && buf != NULL && len >= 0)
    {
        size_t copySize = (size_t) len < sizeof(sendBufCopy[0]) - 1 ? (size_t) len : sizeof(sendBufCopy[0]) - 1;
        memcpy(sendBufCopy[sendCallCount], buf, copySize);
        sendBufCopy[sendCallCount][copySize] = '\0';
        sendLenCopy[sendCallCount]           = (size_t) len;
        sendFlagsCopy[sendCallCount]         = flags;
    }
    sendCallCount++;
    if (sendFails)
    {
        return SOCKET_ERROR;
    }
    if (sendReturnOverride)
    {
        return sendReturnValue;
    }
    return len;
}

int WSAAPI WinsockFake_recv(SOCKET s, char* buf, int len, int flags)
{
    recvCallCount++;
    lastRecvFd    = s;
    lastRecvBuf   = buf;
    lastRecvLen   = (size_t) len;
    lastRecvFlags = flags;
    return recvReturn;
}

int WSAAPI WinsockFake_setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    (void) s;
    (void) optval;
    (void) optlen;
    if (setSockOptCallCount < WINSOCKFAKE_MAX_SETSOCKOPT_CALLS)
    {
        setSockOptLevels[setSockOptCallCount]   = level;
        setSockOptOptnames[setSockOptCallCount] = optname;
    }
    setSockOptCallCount++;
    lastSetSockOptLevel   = level;
    lastSetSockOptOptname = optname;
    return 0;
}

int WSAAPI WinsockFake_getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen)
{
    (void) s;
    getSockOptCallCount++;
    if ((level == IPPROTO_IP) && (optname == IP_MTU))
    {
        if (ipMtuLookupFails)
        {
            return SOCKET_ERROR;
        }
        if ((optval != NULL) && (optlen != NULL) && (*optlen >= (int) sizeof(int)))
        {
            *(int*) optval = ipMtuValue;
            *optlen        = (int) sizeof(int);
        }
        return 0;
    }
    return 0;
}

int WSAAPI WinsockFake_closesocket(SOCKET s)
{
    closeCallCount++;
    lastClosedFd = s;
    return 0;
}

int WSAAPI WinsockFake_getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
{
    (void) service;
    getAddrInfoCallCount++;
    lastGetAddrInfoSocktype = hints ? hints->ai_socktype : 0;
    SafeString_Copy(lastGetAddrInfoHostname, sizeof(lastGetAddrInfoHostname), node ? node : "");
    if (getAddrInfoFails)
    {
        return EAI_FAIL;
    }
    if (node == NULL || res == NULL)
    {
        return EAI_FAIL;
    }
    if (inet_pton(AF_INET, node, &fakeResolvedAddr.sin_addr) != 1)
    {
        return EAI_FAIL;
    }
    *res = &fakeAddrInfo;
    return 0;
}

void WSAAPI WinsockFake_freeaddrinfo(struct addrinfo* res)
{
    (void) res;
    freeAddrInfoCallCount++;
}
