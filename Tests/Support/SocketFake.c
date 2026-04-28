#include "SocketFake.h"
#include "SafeString.h"
#include "SolidSyslog.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

enum
{
    SOCKETFAKE_MAX_BUFFER_SIZE = SOLIDSYSLOG_MAX_MESSAGE_SIZE
};

static bool               sendtoFails;
static bool               nextSendtoShouldFailWithErrno;
static int                nextSendtoErrno;
static int                sendtoCallCount;
static char               lastBufCopy[SOCKETFAKE_MAX_BUFFER_SIZE];
static size_t             lastLen;
static int                lastFlags;
static struct sockaddr_in lastAddr;
static socklen_t          lastAddrLen;
static int                lastSendtoFd;

static int  ipMtuValue;
static bool ipMtuLookupFails;
static int  getSockOptCallCount;

static bool socketFails;
static int  socketCallCount;
static int  socketFd;
static int  lastSocketDomain;
static int  lastSocketType;

enum
{
    SOCKETFAKE_MAX_SEND_CALLS = 8
};

static bool    sendFails;
static int     sendFailOnCall;
static int     sendCallCount;
static bool    sendReturnOverride;
static ssize_t sendReturnValue;
static bool    nextSendShouldFailWithErrno;
static int     nextSendErrno;
static char    sendBufCopy[SOCKETFAKE_MAX_SEND_CALLS][SOCKETFAKE_MAX_BUFFER_SIZE];
static size_t  sendLenCopy[SOCKETFAKE_MAX_SEND_CALLS];
static int     sendFlagsCopy[SOCKETFAKE_MAX_SEND_CALLS];
static int     lastSendFd;

static bool               connectFails;
static int                connectCallCount;
static int                lastConnectFd;
static struct sockaddr_in lastConnectAddr;
static char               lastConnectAddrString[INET_ADDRSTRLEN];

enum
{
    SOCKETFAKE_MAX_SETSOCKOPT_CALLS = 8
};

static int setSockOptCallCount;
static int lastSetSockOptLevel;
static int lastSetSockOptOptname;
static int setSockOptLevels[SOCKETFAKE_MAX_SETSOCKOPT_CALLS];
static int setSockOptOptnames[SOCKETFAKE_MAX_SETSOCKOPT_CALLS];

static int closeCallCount;
static int lastClosedFd;

static int         recvCallCount;
static ssize_t     recvReturn;
static int         lastRecvFd;
static const void* lastRecvBuf;
static size_t      lastRecvLen;
static int         lastRecvFlags;

static char lastAddrString[INET_ADDRSTRLEN];

static bool               getAddrInfoFails;
static int                getAddrInfoCallCount;
static char               lastGetAddrInfoHostname[256];
static int                lastGetAddrInfoSocktype;
static struct sockaddr_in fakeResolvedAddr;
static struct addrinfo    fakeAddrInfo;
static int                freeAddrInfoCallCount;

void SocketFake_Reset(void)
{
    /* Reset errno so a stale value from a prior test (notably EINTR set via
     * FailNextSendWithErrno) cannot leak into the next test's retry loop. */
    errno                         = 0;
    sendtoFails                   = false;
    nextSendtoShouldFailWithErrno = false;
    nextSendtoErrno               = 0;
    sendtoCallCount               = 0;
    lastBufCopy[0]                = '\0';
    lastLen                       = 0;
    lastFlags                     = 0;
    lastAddr                      = (struct sockaddr_in) {0};
    lastAddrLen                   = 0;
    lastSendtoFd                  = -1;
    sendFails                     = false;
    sendFailOnCall                = -1;
    sendCallCount                 = 0;
    sendReturnOverride            = false;
    sendReturnValue               = 0;
    nextSendShouldFailWithErrno   = false;
    nextSendErrno                 = 0;
    for (int i = 0; i < SOCKETFAKE_MAX_SEND_CALLS; i++)
    {
        sendBufCopy[i][0] = '\0';
        sendLenCopy[i]    = 0;
        sendFlagsCopy[i]  = 0;
    }
    lastSendFd               = -1;
    connectFails             = false;
    connectCallCount         = 0;
    lastConnectFd            = -1;
    lastConnectAddr          = (struct sockaddr_in) {0};
    lastConnectAddrString[0] = '\0';
    setSockOptCallCount      = 0;
    lastSetSockOptLevel      = 0;
    lastSetSockOptOptname    = 0;
    for (int i = 0; i < SOCKETFAKE_MAX_SETSOCKOPT_CALLS; i++)
    {
        setSockOptLevels[i]   = 0;
        setSockOptOptnames[i] = 0;
    }
    getSockOptCallCount = 0;
    ipMtuValue          = 0;
    ipMtuLookupFails    = false;
    socketFails         = false;
    socketCallCount     = 0;
    socketFd            = -1;
    lastSocketDomain    = 0;
    lastSocketType      = 0;
    closeCallCount      = 0;
    lastClosedFd        = -1;
    recvCallCount       = 0;
    recvReturn          = 0;
    lastRecvFd          = -1;
    lastRecvBuf         = NULL;
    lastRecvLen         = 0;
    lastRecvFlags       = 0;
    lastAddrString[0]   = '\0';

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

void SocketFake_SetSendtoFails(bool fails)
{
    sendtoFails = fails;
}

void SocketFake_FailNextSendtoWithErrno(int errnoValue)
{
    nextSendtoShouldFailWithErrno = true;
    nextSendtoErrno               = errnoValue;
}

void SocketFake_SetIpMtu(int mtu)
{
    ipMtuValue = mtu;
}

void SocketFake_SetIpMtuLookupFails(bool fails)
{
    ipMtuLookupFails = fails;
}

int SocketFake_GetSockOptCallCount(void)
{
    return getSockOptCallCount;
}

/* sendto accessors */

int SocketFake_SendtoCallCount(void)
{
    return sendtoCallCount;
}

const void* SocketFake_LastBuf(void)
{
    return lastBufCopy;
}

const char* SocketFake_LastBufAsString(void)
{
    return lastBufCopy;
}

size_t SocketFake_LastLen(void)
{
    return lastLen;
}

int SocketFake_LastFlags(void)
{
    return lastFlags;
}

int SocketFake_LastAddrFamily(void)
{
    return lastAddr.sin_family;
}

int SocketFake_LastPort(void)
{
    return ntohs(lastAddr.sin_port);
}

const char* SocketFake_LastAddrAsString(void)
{
    inet_ntop(AF_INET, &lastAddr.sin_addr, lastAddrString, sizeof(lastAddrString));
    return lastAddrString;
}

socklen_t SocketFake_LastAddrLen(void)
{
    return lastAddrLen;
}

int SocketFake_LastSendtoFd(void)
{
    return lastSendtoFd;
}

/* socket configuration */

void SocketFake_SetSocketFails(bool fails)
{
    socketFails = fails;
}

/* socket accessors */

int SocketFake_SocketCallCount(void)
{
    return socketCallCount;
}

int SocketFake_SocketFd(void)
{
    return socketFd;
}

int SocketFake_SocketDomain(void)
{
    return lastSocketDomain;
}

int SocketFake_SocketType(void)
{
    return lastSocketType;
}

/* send configuration */

void SocketFake_SetSendFails(bool fails)
{
    sendFails = fails;
}

void SocketFake_FailSendOnCall(int callNumber)
{
    sendFailOnCall = callNumber;
}

void SocketFake_SetSendReturn(ssize_t value)
{
    sendReturnOverride = true;
    sendReturnValue    = value;
}

void SocketFake_FailNextSendWithErrno(int errnoValue)
{
    nextSendShouldFailWithErrno = true;
    nextSendErrno               = errnoValue;
}

/* send accessors */

int SocketFake_SendCallCount(void)
{
    return sendCallCount;
}

const char* SocketFake_SendBufAsString(int callIndex)
{
    if (callIndex < 0 || callIndex >= SOCKETFAKE_MAX_SEND_CALLS)
    {
        return "";
    }
    return sendBufCopy[callIndex];
}

size_t SocketFake_SendLen(int callIndex)
{
    if (callIndex < 0 || callIndex >= SOCKETFAKE_MAX_SEND_CALLS)
    {
        return 0;
    }
    return sendLenCopy[callIndex];
}

int SocketFake_LastSendFd(void)
{
    return lastSendFd;
}

int SocketFake_SendFlags(int callIndex)
{
    if (callIndex < 0 || callIndex >= SOCKETFAKE_MAX_SEND_CALLS)
    {
        return 0;
    }
    return sendFlagsCopy[callIndex];
}

/* connect configuration */

void SocketFake_SetConnectFails(bool fails)
{
    connectFails = fails;
}

/* connect accessors */

int SocketFake_ConnectCallCount(void)
{
    return connectCallCount;
}

int SocketFake_LastConnectFd(void)
{
    return lastConnectFd;
}

int SocketFake_LastConnectPort(void)
{
    return ntohs(lastConnectAddr.sin_port);
}

const char* SocketFake_LastConnectAddrAsString(void)
{
    inet_ntop(AF_INET, &lastConnectAddr.sin_addr, lastConnectAddrString, sizeof(lastConnectAddrString));
    return lastConnectAddrString;
}

/* setsockopt accessors */

int SocketFake_SetSockOptCallCount(void)
{
    return setSockOptCallCount;
}

int SocketFake_LastSetSockOptLevel(void)
{
    return lastSetSockOptLevel;
}

int SocketFake_LastSetSockOptOptname(void)
{
    return lastSetSockOptOptname;
}

bool SocketFake_HasSetSockOpt(int level, int optname)
{
    int recorded = setSockOptCallCount < SOCKETFAKE_MAX_SETSOCKOPT_CALLS ? setSockOptCallCount : SOCKETFAKE_MAX_SETSOCKOPT_CALLS;
    for (int i = 0; i < recorded; i++)
    {
        if (setSockOptLevels[i] == level && setSockOptOptnames[i] == optname)
        {
            return true;
        }
    }
    return false;
}

/* close accessors */

int SocketFake_CloseCallCount(void)
{
    return closeCallCount;
}

int SocketFake_LastClosedFd(void)
{
    return lastClosedFd;
}

/* recv configuration */

void SocketFake_SetRecvReturn(ssize_t value)
{
    recvReturn = value;
}

/* recv accessors */

int SocketFake_RecvCallCount(void)
{
    return recvCallCount;
}

int SocketFake_LastRecvFd(void)
{
    return lastRecvFd;
}

const void* SocketFake_LastRecvBuf(void)
{
    return lastRecvBuf;
}

size_t SocketFake_LastRecvLen(void)
{
    return lastRecvLen;
}

int SocketFake_LastRecvFlags(void)
{
    return lastRecvFlags;
}

/* getaddrinfo configuration */

void SocketFake_SetGetAddrInfoFails(bool fails)
{
    getAddrInfoFails = fails;
}

/* getaddrinfo accessors */

int SocketFake_GetAddrInfoCallCount(void)
{
    return getAddrInfoCallCount;
}

const char* SocketFake_LastGetAddrInfoHostname(void)
{
    return lastGetAddrInfoHostname;
}

int SocketFake_LastGetAddrInfoSocktype(void)
{
    return lastGetAddrInfoSocktype;
}

/* freeaddrinfo accessors */

int SocketFake_FreeAddrInfoCallCount(void)
{
    return freeAddrInfoCallCount;
}

/* POSIX strong-symbol fakes */

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- POSIX API; signature is fixed
int socket(int domain, int type, int protocol)
{
    (void) protocol;
    socketCallCount++;
    lastSocketDomain = domain;
    lastSocketType   = type;
    if (socketFails)
    {
        socketFd = -1;
    }
    else
    {
        socketFd = socketCallCount; /* deterministic fake fd */
    }
    return socketFd;
}

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; parameter names differ from glibc internal names
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen)
// clang-format on
{
    sendtoCallCount++;
    lastSendtoFd    = sockfd;
    size_t copySize = len < sizeof(lastBufCopy) - 1 ? len : sizeof(lastBufCopy) - 1;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize; memcpy_s is not portable
    memcpy(lastBufCopy, buf, copySize);
    lastBufCopy[copySize] = '\0';
    lastLen               = len;
    lastFlags             = flags;
    lastAddr              = *(const struct sockaddr_in*) dest_addr;
    lastAddrLen           = addrlen;
    if (nextSendtoShouldFailWithErrno)
    {
        errno                         = nextSendtoErrno;
        nextSendtoShouldFailWithErrno = false;
        return (ssize_t) -1;
    }
    if (sendtoFails)
    {
        /* Set a deterministic errno so a stale value (notably EMSGSIZE
         * left by a prior FailNextSendtoWithErrno call) cannot leak
         * into the production code's error-classification logic. */
        errno = EIO;
        return (ssize_t) -1;
    }
    return (ssize_t) len;
}

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; parameter names differ from glibc internal names
int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
// clang-format on
{
    (void) sockfd;
    getSockOptCallCount++;
    if ((level == IPPROTO_IP) && (optname == IP_MTU))
    {
        if (ipMtuLookupFails)
        {
            return -1;
        }
        if ((optval != NULL) && (optlen != NULL) && (*optlen >= sizeof(int)))
        {
            *(int*) optval = ipMtuValue;
            *optlen        = sizeof(int);
        }
        return 0;
    }
    return 0;
}

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; parameter names differ from glibc internal names
ssize_t send(int sockfd, const void* buf, size_t len, int flags)
// clang-format on
{
    lastSendFd = sockfd;
    if (sendCallCount < SOCKETFAKE_MAX_SEND_CALLS)
    {
        size_t copySize = len < sizeof(sendBufCopy[0]) - 1 ? len : sizeof(sendBufCopy[0]) - 1;
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize
        memcpy(sendBufCopy[sendCallCount], buf, copySize);
        sendBufCopy[sendCallCount][copySize] = '\0';
        sendLenCopy[sendCallCount]           = len;
        sendFlagsCopy[sendCallCount]         = flags;
    }
    bool failThisCall = sendFails || (sendFailOnCall == sendCallCount);
    sendCallCount++;
    if (nextSendShouldFailWithErrno)
    {
        errno                       = nextSendErrno;
        nextSendShouldFailWithErrno = false;
        return (ssize_t) -1;
    }
    if (failThisCall)
    {
        /* Set a non-retryable errno so the production EINTR-retry loop
         * cannot wedge if errno happened to be EINTR from a prior test. */
        errno = EIO;
        return (ssize_t) -1;
    }
    if (sendReturnOverride)
    {
        return sendReturnValue;
    }
    return (ssize_t) len;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name) -- POSIX API; parameter names differ from glibc internal names
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    (void) addrlen;
    connectCallCount++;
    lastConnectFd   = sockfd;
    lastConnectAddr = *(const struct sockaddr_in*) addr;
    return connectFails ? -1 : 0;
}

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; parameter names differ from glibc internal names
int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
// clang-format on
{
    (void) sockfd;
    (void) optval;
    (void) optlen;
    if (setSockOptCallCount < SOCKETFAKE_MAX_SETSOCKOPT_CALLS)
    {
        setSockOptLevels[setSockOptCallCount]   = level;
        setSockOptOptnames[setSockOptCallCount] = optname;
    }
    setSockOptCallCount++;
    lastSetSockOptLevel   = level;
    lastSetSockOptOptname = optname;
    return 0;
}

int close(int fd)
{
    closeCallCount++;
    lastClosedFd = fd;
    return 0;
}

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; names differ from glibc internal names
ssize_t recv(int sockfd, void* buf, size_t len, int flags)
// clang-format on
{
    recvCallCount++;
    lastRecvFd    = sockfd;
    lastRecvBuf   = buf;
    lastRecvLen   = len;
    lastRecvFlags = flags;
    return recvReturn;
}

// clang-format off
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; parameter names differ from glibc internal names
int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
// clang-format on
{
    (void) service;
    getAddrInfoCallCount++;
    lastGetAddrInfoSocktype = hints ? hints->ai_socktype : 0;
    SafeString_Copy(lastGetAddrInfoHostname, sizeof(lastGetAddrInfoHostname), node ? node : "");
    if (getAddrInfoFails)
    {
        return EAI_FAIL;
    }
    inet_pton(AF_INET, node, &fakeResolvedAddr.sin_addr);
    *res = &fakeAddrInfo;
    return 0;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name) -- POSIX API; parameter names differ from glibc internal names
void freeaddrinfo(struct addrinfo* res)
{
    (void) res;
    freeAddrInfoCallCount++;
}
