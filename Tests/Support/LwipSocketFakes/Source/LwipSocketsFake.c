#include "LwipSocketsFake.h"

#include <errno.h>
#include <stdbool.h>
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

static unsigned fcntlCallCount = 0U;
static int fcntlResult = 0;
static int lastFcntlSocket = 0;
static int lastFcntlCommand = 0;
static int lastFcntlValue = 0;
static unsigned fcntlCallsBeforeConnect = 0U;

static unsigned connectCallCount = 0U;
static int connectResult = 0;
static int connectErrno = 0;
static int lastConnectSocket = 0;
static struct sockaddr_in lastConnectAddress;
static socklen_t lastConnectAddressLength = 0;

static unsigned sendCallCount = 0U;
static ssize_t sendResult = 0;
static bool sendResultProgrammed = false;
static int sendErrno = 0;
static int lastSendSocket = 0;
static char lastSendPayload[FAKE_PAYLOAD_CAPACITY];
static size_t lastSendSize = 0U;
static int lastSendFlags = 0;

static unsigned recvCallCount = 0U;
static ssize_t recvResult = 0;
static bool recvResultProgrammed = false;
static int recvErrno = 0;
static char recvPayload[FAKE_PAYLOAD_CAPACITY];
static size_t recvPayloadSize = 0U;
static int lastRecvSocket = 0;
static size_t lastRecvSize = 0U;
static int lastRecvFlags = 0;

static unsigned selectCallCount = 0U;
static int selectResult = 1;
static bool selectSignalsException = false;
static int lastSelectMaxFdPlusOne = 0;
static int lastSelectWriteDescriptor = -1;
static int lastSelectExceptionDescriptor = -1;
static unsigned lastSelectTimeoutMs = 0U;

enum
{
    FAKE_SOCKOPT_CAPACITY = 16
};

struct FakeSockOpt
{
    int Level;
    int Name;
    int Value;
};

static unsigned setSockOptCallCount = 0U;
static struct FakeSockOpt setSockOpts[FAKE_SOCKOPT_CAPACITY];
static int refusedSockOptLevel = -1;
static int refusedSockOptName = -1;
static bool refusesEverySockOpt = false;

static unsigned getSockOptCallCount = 0U;
static int socketError = 0;
static int lastGetSockOptSocket = 0;
static int lastGetSockOptLevel = 0;
static int lastGetSockOptName = 0;

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

    fcntlCallCount = 0U;
    fcntlResult = 0;
    lastFcntlSocket = 0;
    lastFcntlCommand = 0;
    lastFcntlValue = 0;
    fcntlCallsBeforeConnect = 0U;

    connectCallCount = 0U;
    connectResult = 0;
    connectErrno = 0;
    lastConnectSocket = 0;
    (void) memset(&lastConnectAddress, 0, sizeof(lastConnectAddress));
    lastConnectAddressLength = 0;

    sendCallCount = 0U;
    sendResult = 0;
    sendResultProgrammed = false;
    sendErrno = 0;
    lastSendSocket = 0;
    (void) memset(lastSendPayload, 0, sizeof(lastSendPayload));
    lastSendSize = 0U;
    lastSendFlags = 0;

    recvCallCount = 0U;
    recvResult = 0;
    recvResultProgrammed = false;
    recvErrno = 0;
    (void) memset(recvPayload, 0, sizeof(recvPayload));
    recvPayloadSize = 0U;
    lastRecvSocket = 0;
    lastRecvSize = 0U;
    lastRecvFlags = 0;

    selectCallCount = 0U;
    selectResult = 1;
    selectSignalsException = false;
    lastSelectMaxFdPlusOne = 0;
    lastSelectWriteDescriptor = -1;
    lastSelectExceptionDescriptor = -1;
    lastSelectTimeoutMs = 0U;

    setSockOptCallCount = 0U;
    (void) memset(setSockOpts, 0, sizeof(setSockOpts));
    refusedSockOptLevel = -1;
    refusedSockOptName = -1;
    refusesEverySockOpt = false;

    getSockOptCallCount = 0U;
    socketError = 0;
    lastGetSockOptSocket = 0;
    lastGetSockOptLevel = 0;
    lastGetSockOptName = 0;

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

void LwipSocketsFake_SetFcntlResult(int result)
{
    fcntlResult = result;
}

unsigned LwipSocketsFake_FcntlCallCount(void)
{
    return fcntlCallCount;
}

int LwipSocketsFake_LastFcntlSocket(void)
{
    return lastFcntlSocket;
}

int LwipSocketsFake_LastFcntlCommand(void)
{
    return lastFcntlCommand;
}

int LwipSocketsFake_LastFcntlValue(void)
{
    return lastFcntlValue;
}

unsigned LwipSocketsFake_FcntlCallsBeforeConnect(void)
{
    return fcntlCallsBeforeConnect;
}

void LwipSocketsFake_SetConnectResult(int result, int err)
{
    connectResult = result;
    connectErrno = err;
}

unsigned LwipSocketsFake_ConnectCallCount(void)
{
    return connectCallCount;
}

int LwipSocketsFake_LastConnectSocket(void)
{
    return lastConnectSocket;
}

const struct sockaddr_in* LwipSocketsFake_LastConnectAddress(void)
{
    return &lastConnectAddress;
}

socklen_t LwipSocketsFake_LastConnectAddressLength(void)
{
    return lastConnectAddressLength;
}

void LwipSocketsFake_SetSendResult(ssize_t result, int err)
{
    sendResult = result;
    sendResultProgrammed = true;
    sendErrno = err;
}

unsigned LwipSocketsFake_SendCallCount(void)
{
    return sendCallCount;
}

int LwipSocketsFake_LastSendSocket(void)
{
    return lastSendSocket;
}

const void* LwipSocketsFake_LastSendPayload(void)
{
    return lastSendPayload;
}

size_t LwipSocketsFake_LastSendSize(void)
{
    return lastSendSize;
}

int LwipSocketsFake_LastSendFlags(void)
{
    return lastSendFlags;
}

void LwipSocketsFake_SetRecvPayload(const char* payload)
{
    recvPayloadSize = strlen(payload);
    (void) memcpy(recvPayload, payload, recvPayloadSize);
}

void LwipSocketsFake_SetRecvResult(ssize_t result, int err)
{
    recvResult = result;
    recvResultProgrammed = true;
    recvErrno = err;
}

unsigned LwipSocketsFake_RecvCallCount(void)
{
    return recvCallCount;
}

int LwipSocketsFake_LastRecvSocket(void)
{
    return lastRecvSocket;
}

size_t LwipSocketsFake_LastRecvSize(void)
{
    return lastRecvSize;
}

int LwipSocketsFake_LastRecvFlags(void)
{
    return lastRecvFlags;
}

void LwipSocketsFake_SetSelectResult(int result)
{
    selectResult = result;
}

void LwipSocketsFake_SetSelectSignalsException(void)
{
    selectSignalsException = true;
}

unsigned LwipSocketsFake_SelectCallCount(void)
{
    return selectCallCount;
}

int LwipSocketsFake_LastSelectMaxFdPlusOne(void)
{
    return lastSelectMaxFdPlusOne;
}

int LwipSocketsFake_LastSelectWriteDescriptor(void)
{
    return lastSelectWriteDescriptor;
}

int LwipSocketsFake_LastSelectExceptionDescriptor(void)
{
    return lastSelectExceptionDescriptor;
}

unsigned LwipSocketsFake_LastSelectTimeoutMs(void)
{
    return lastSelectTimeoutMs;
}

void LwipSocketsFake_SetSockOptRefuses(int level, int optname)
{
    refusedSockOptLevel = level;
    refusedSockOptName = optname;
}

void LwipSocketsFake_SetSockOptRefusesEverything(void)
{
    refusesEverySockOpt = true;
}

unsigned LwipSocketsFake_SetSockOptCallCount(void)
{
    return setSockOptCallCount;
}

bool LwipSocketsFake_SockOptWasSetTo(int level, int optname, int value)
{
    bool found = false;
    for (unsigned i = 0U; (i < setSockOptCallCount) && (i < FAKE_SOCKOPT_CAPACITY) && !found; i++)
    {
        found = (setSockOpts[i].Level == level) && (setSockOpts[i].Name == optname) && (setSockOpts[i].Value == value);
    }
    return found;
}

void LwipSocketsFake_SetSocketError(int err)
{
    socketError = err;
}

unsigned LwipSocketsFake_GetSockOptCallCount(void)
{
    return getSockOptCallCount;
}

int LwipSocketsFake_LastGetSockOptSocket(void)
{
    return lastGetSockOptSocket;
}

int LwipSocketsFake_LastGetSockOptLevel(void)
{
    return lastGetSockOptLevel;
}

int LwipSocketsFake_LastGetSockOptName(void)
{
    return lastGetSockOptName;
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

int lwip_fcntl(int s, int cmd, int val)
{
    fcntlCallCount++;
    lastFcntlSocket = s;
    lastFcntlCommand = cmd;
    lastFcntlValue = val;
    return fcntlResult;
}

int lwip_connect(int s, const struct sockaddr* name, socklen_t namelen)
{
    connectCallCount++;
    fcntlCallsBeforeConnect = fcntlCallCount;
    lastConnectSocket = s;
    lastConnectAddressLength = namelen;
    if (name != NULL)
    {
        (void) memcpy(&lastConnectAddress, name, sizeof(lastConnectAddress));
    }
    if (connectResult < 0)
    {
        errno = connectErrno;
    }
    return connectResult;
}

/* Records which descriptor the caller watched, then answers the programmed
   result the way the sockets layer does: the sets come back holding only what
   is ready. */
static int LwipSocketsFake_FirstSetDescriptor(const fd_set* set, int maxFdPlusOne)
{
    int found = -1;
    for (int fd = 0; (fd < maxFdPlusOne) && (found < 0); fd++)
    {
        if ((set != NULL) && FD_ISSET(fd, set))
        {
            found = fd;
        }
    }
    return found;
}

int lwip_select(int maxfdp1, fd_set* readset, fd_set* writeset, fd_set* exceptset, struct timeval* timeout)
{
    selectCallCount++;
    lastSelectMaxFdPlusOne = maxfdp1;
    lastSelectWriteDescriptor = LwipSocketsFake_FirstSetDescriptor(writeset, maxfdp1);
    lastSelectExceptionDescriptor = LwipSocketsFake_FirstSetDescriptor(exceptset, maxfdp1);
    lastSelectTimeoutMs = (timeout != NULL) ? (unsigned) ((timeout->tv_sec * 1000L) + (timeout->tv_usec / 1000L)) : 0U;
    (void) readset;

    int watched = lastSelectWriteDescriptor;
    if (writeset != NULL)
    {
        FD_ZERO(writeset);
    }
    if (exceptset != NULL)
    {
        FD_ZERO(exceptset);
    }
    if ((selectResult > 0) && (watched >= 0))
    {
        if (selectSignalsException && (exceptset != NULL))
        {
            FD_SET(watched, exceptset);
        }
        else if (writeset != NULL)
        {
            FD_SET(watched, writeset);
        }
        else
        {
            /* nothing to report back */
        }
    }
    return selectResult;
}

int lwip_getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen)
{
    getSockOptCallCount++;
    lastGetSockOptSocket = s;
    lastGetSockOptLevel = level;
    lastGetSockOptName = optname;
    if ((optval != NULL) && (optlen != NULL) && (*optlen >= sizeof(int)))
    {
        *(int*) optval = socketError;
        *optlen = sizeof(int);
    }
    return 0;
}

ssize_t lwip_send(int s, const void* dataptr, size_t size, int flags)
{
    sendCallCount++;
    lastSendSocket = s;
    lastSendSize = size;
    lastSendFlags = flags;
    if (size <= sizeof(lastSendPayload))
    {
        (void) memcpy(lastSendPayload, dataptr, size);
    }

    ssize_t result = (ssize_t) size;
    if (sendResultProgrammed)
    {
        result = sendResult;
        if (result < 0)
        {
            errno = sendErrno;
        }
    }
    return result;
}

ssize_t lwip_recv(int s, void* mem, size_t len, int flags)
{
    recvCallCount++;
    lastRecvSocket = s;
    lastRecvSize = len;
    lastRecvFlags = flags;

    ssize_t result = (ssize_t) recvPayloadSize;
    if (recvResultProgrammed)
    {
        result = recvResult;
    }
    if (result > 0)
    {
        size_t copied = (recvPayloadSize < len) ? recvPayloadSize : len;
        (void) memcpy(mem, recvPayload, copied);
    }
    else if (result < 0)
    {
        errno = recvErrno;
    }
    else
    {
        /* a peer close hands back nothing */
    }
    return result;
}

int lwip_setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen)
{
    (void) s;
    if (setSockOptCallCount < FAKE_SOCKOPT_CAPACITY)
    {
        setSockOpts[setSockOptCallCount].Level = level;
        setSockOpts[setSockOptCallCount].Name = optname;
        setSockOpts[setSockOptCallCount].Value =
            ((optval != NULL) && (optlen >= sizeof(int))) ? *(const int*) optval : 0;
    }
    setSockOptCallCount++;

    int result = 0;
    if (refusesEverySockOpt || ((level == refusedSockOptLevel) && (optname == refusedSockOptName)))
    {
        errno = ENOPROTOOPT;
        result = -1;
    }
    return result;
}
