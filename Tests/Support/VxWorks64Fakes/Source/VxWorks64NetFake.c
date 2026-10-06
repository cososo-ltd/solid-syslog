#include "vxWorks.h"

#include "VxWorks64NetFake.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/times.h>

#include "hostLib.h"
#include "inetLib.h"
#include "sockLib.h"

static const char* VxWorks64NetFake_Hostname = "";
static char VxWorks64NetFake_SetHostnameBuffer[MAXHOSTNAMELEN + 1];
static bool VxWorks64NetFake_GethostnameFails = false;
static int VxWorks64NetFake_GethostnameLength = 0;
static unsigned long VxWorks64NetFake_InetAddrReturn = 0UL;
static const char* VxWorks64NetFake_InetAddrString = NULL;
static int VxWorks64NetFake_HostGetByNameReturn = 0;
static unsigned VxWorks64NetFake_HostGetByNameCount = 0U;
static const char* VxWorks64NetFake_HostGetByNameName = NULL;

/* What socket hands back when it succeeds: any descriptor other than zero, so
 * an adapter that assumed a fixed one would be caught. */
static const int VxWorks64NetFake_Fd = 7;

static bool VxWorks64NetFake_SocketFails = false;
static unsigned VxWorks64NetFake_SocketCount = 0U;
static int VxWorks64NetFake_SocketDomain = 0;
static int VxWorks64NetFake_SocketType = 0;
static int VxWorks64NetFake_SocketProtocol = -1;
static int VxWorks64NetFake_SendtoErrno = 0;
static unsigned VxWorks64NetFake_SendtoCount = 0U;
static int VxWorks64NetFake_SendtoFd = -1;
static const char* VxWorks64NetFake_SendtoBuf = NULL;
static char VxWorks64NetFake_SendtoPayload[2048];
static int VxWorks64NetFake_SendtoLen = 0;
static int VxWorks64NetFake_SendtoFlags = -1;
static const struct sockaddr* VxWorks64NetFake_SendtoTo = NULL;
static int VxWorks64NetFake_SendtoToLen = 0;
static int VxWorks64NetFake_ConnectErrno = 0;
static unsigned VxWorks64NetFake_ConnectCount = 0U;
static int VxWorks64NetFake_ConnectFd = -1;
static const struct sockaddr* VxWorks64NetFake_ConnectAddress = NULL;
static int VxWorks64NetFake_ConnectAddressLength = 0;
static bool VxWorks64NetFake_ConnectBounded = false;
static long VxWorks64NetFake_ConnectTimeoutSeconds = -1L;
static long VxWorks64NetFake_ConnectTimeoutMicroseconds = -1L;

enum
{
    VXWORKS64NETFAKE_MAX_SOCKET_OPTIONS = 8
};

struct VxWorks64NetFake_SocketOption
{
    int Fd;
    int Level;
    int Optname;
    int Value;
    int Optlen;
};

static int VxWorks64NetFake_RefusedLevel = -1;
static int VxWorks64NetFake_RefusedOptname = -1;
static unsigned VxWorks64NetFake_SetsockoptCount = 0U;
static struct VxWorks64NetFake_SocketOption VxWorks64NetFake_SocketOptions[VXWORKS64NETFAKE_MAX_SOCKET_OPTIONS];
static int VxWorks64NetFake_SendErrno = 0;
static int VxWorks64NetFake_SendLimit = -1;
static unsigned VxWorks64NetFake_SendCount = 0U;
static int VxWorks64NetFake_SendFd = -1;
static const char* VxWorks64NetFake_SendBuf = NULL;
static int VxWorks64NetFake_SendLen = 0;
static int VxWorks64NetFake_SendFlags = -1;
static int VxWorks64NetFake_RecvErrno = 0;
static const char* VxWorks64NetFake_RecvData = NULL;
static int VxWorks64NetFake_RecvDataLength = 0;
static unsigned VxWorks64NetFake_RecvCount = 0U;
static int VxWorks64NetFake_RecvFd = -1;
static const char* VxWorks64NetFake_RecvBuf = NULL;
static int VxWorks64NetFake_RecvLen = 0;
static int VxWorks64NetFake_RecvFlags = -1;

void VxWorks64NetFake_Reset(void)
{
    VxWorks64NetFake_Hostname = "";
    VxWorks64NetFake_GethostnameFails = false;
    VxWorks64NetFake_GethostnameLength = 0;
    VxWorks64NetFake_SendtoPayload[0] = '\0';
    VxWorks64NetFake_InetAddrReturn = 0UL;
    VxWorks64NetFake_InetAddrString = NULL;
    VxWorks64NetFake_HostGetByNameReturn = 0;
    VxWorks64NetFake_HostGetByNameCount = 0U;
    VxWorks64NetFake_HostGetByNameName = NULL;
    VxWorks64NetFake_SocketFails = false;
    VxWorks64NetFake_SocketCount = 0U;
    VxWorks64NetFake_SocketDomain = 0;
    VxWorks64NetFake_SocketType = 0;
    VxWorks64NetFake_SocketProtocol = -1;
    VxWorks64NetFake_SendtoErrno = 0;
    VxWorks64NetFake_SendtoCount = 0U;
    VxWorks64NetFake_SendtoFd = -1;
    VxWorks64NetFake_SendtoBuf = NULL;
    VxWorks64NetFake_SendtoLen = 0;
    VxWorks64NetFake_SendtoFlags = -1;
    VxWorks64NetFake_SendtoTo = NULL;
    VxWorks64NetFake_SendtoToLen = 0;
    VxWorks64NetFake_ConnectErrno = 0;
    VxWorks64NetFake_ConnectCount = 0U;
    VxWorks64NetFake_ConnectFd = -1;
    VxWorks64NetFake_ConnectAddress = NULL;
    VxWorks64NetFake_ConnectAddressLength = 0;
    VxWorks64NetFake_ConnectBounded = false;
    VxWorks64NetFake_ConnectTimeoutSeconds = -1L;
    VxWorks64NetFake_ConnectTimeoutMicroseconds = -1L;
    VxWorks64NetFake_RefusedLevel = -1;
    VxWorks64NetFake_RefusedOptname = -1;
    VxWorks64NetFake_SetsockoptCount = 0U;
    VxWorks64NetFake_SendErrno = 0;
    VxWorks64NetFake_SendLimit = -1;
    VxWorks64NetFake_SendCount = 0U;
    VxWorks64NetFake_SendFd = -1;
    VxWorks64NetFake_SendBuf = NULL;
    VxWorks64NetFake_SendLen = 0;
    VxWorks64NetFake_SendFlags = -1;
    VxWorks64NetFake_RecvErrno = EWOULDBLOCK;
    VxWorks64NetFake_RecvData = NULL;
    VxWorks64NetFake_RecvDataLength = 0;
    VxWorks64NetFake_RecvCount = 0U;
    VxWorks64NetFake_RecvFd = -1;
    VxWorks64NetFake_RecvBuf = NULL;
    VxWorks64NetFake_RecvLen = 0;
    VxWorks64NetFake_RecvFlags = -1;
}

void VxWorks64NetFake_SetInetAddrReturn(unsigned long value)
{
    VxWorks64NetFake_InetAddrReturn = value;
}

const char* VxWorks64NetFake_LastInetAddrString(void)
{
    return VxWorks64NetFake_InetAddrString;
}

void VxWorks64NetFake_SetHostGetByNameReturn(int value)
{
    VxWorks64NetFake_HostGetByNameReturn = value;
}

unsigned VxWorks64NetFake_HostGetByNameCallCount(void)
{
    return VxWorks64NetFake_HostGetByNameCount;
}

const char* VxWorks64NetFake_LastHostGetByNameName(void)
{
    return VxWorks64NetFake_HostGetByNameName;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
unsigned long inet_addr(char* inetString)
{
    VxWorks64NetFake_InetAddrString = inetString;
    return VxWorks64NetFake_InetAddrReturn;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
int hostGetByName(char* name)
{
    VxWorks64NetFake_HostGetByNameCount++;
    VxWorks64NetFake_HostGetByNameName = name;
    return VxWorks64NetFake_HostGetByNameReturn;
}

void VxWorks64NetFake_SetHostname(const char* name)
{
    VxWorks64NetFake_Hostname = name;
}

void VxWorks64NetFake_FailGethostname(void)
{
    VxWorks64NetFake_GethostnameFails = true;
}

int VxWorks64NetFake_LastGethostnameLength(void)
{
    return VxWorks64NetFake_GethostnameLength;
}

int gethostname(char* name, int nameLen)
{
    VxWorks64NetFake_GethostnameLength = nameLen;
    const char* answer = VxWorks64NetFake_GethostnameFails ? "unspecified" : VxWorks64NetFake_Hostname;
    size_t length = strlen(answer) + 1U;
    size_t copied = (length < (size_t) nameLen) ? length : (size_t) nameLen;
    (void) memcpy(name, answer, copied);
    return VxWorks64NetFake_GethostnameFails ? ERROR : OK;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
/* Keeps the name, as the kernel does, for gethostname to answer. */
int sethostname(char* name, int nameLen)
{
    size_t length = ((size_t) nameLen < MAXHOSTNAMELEN) ? (size_t) nameLen : MAXHOSTNAMELEN;
    (void) memcpy(VxWorks64NetFake_SetHostnameBuffer, name, length);
    VxWorks64NetFake_SetHostnameBuffer[length] = '\0';
    VxWorks64NetFake_Hostname = VxWorks64NetFake_SetHostnameBuffer;
    return OK;
}

void VxWorks64NetFake_SetSocketFails(bool fails)
{
    VxWorks64NetFake_SocketFails = fails;
}

unsigned VxWorks64NetFake_SocketCallCount(void)
{
    return VxWorks64NetFake_SocketCount;
}

int VxWorks64NetFake_LastSocketDomain(void)
{
    return VxWorks64NetFake_SocketDomain;
}

int VxWorks64NetFake_LastSocketType(void)
{
    return VxWorks64NetFake_SocketType;
}

int VxWorks64NetFake_LastSocketProtocol(void)
{
    return VxWorks64NetFake_SocketProtocol;
}

int VxWorks64NetFake_SocketFd(void)
{
    return VxWorks64NetFake_Fd;
}

void VxWorks64NetFake_FailSendtoWithErrno(int errnoValue)
{
    VxWorks64NetFake_SendtoErrno = errnoValue;
}

unsigned VxWorks64NetFake_SendtoCallCount(void)
{
    return VxWorks64NetFake_SendtoCount;
}

int VxWorks64NetFake_LastSendtoFd(void)
{
    return VxWorks64NetFake_SendtoFd;
}

const char* VxWorks64NetFake_LastSendtoBuf(void)
{
    return VxWorks64NetFake_SendtoBuf;
}

const char* VxWorks64NetFake_LastSendtoPayload(void)
{
    return VxWorks64NetFake_SendtoPayload;
}

int VxWorks64NetFake_LastSendtoLen(void)
{
    return VxWorks64NetFake_SendtoLen;
}

int VxWorks64NetFake_LastSendtoFlags(void)
{
    return VxWorks64NetFake_SendtoFlags;
}

const struct sockaddr* VxWorks64NetFake_LastSendtoTo(void)
{
    return VxWorks64NetFake_SendtoTo;
}

int VxWorks64NetFake_LastSendtoToLen(void)
{
    return VxWorks64NetFake_SendtoToLen;
}

int socket(int domain, int type, int protocol)
{
    VxWorks64NetFake_SocketCount++;
    VxWorks64NetFake_SocketDomain = domain;
    VxWorks64NetFake_SocketType = type;
    VxWorks64NetFake_SocketProtocol = protocol;
    return VxWorks64NetFake_SocketFails ? ERROR : VxWorks64NetFake_Fd;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
int sendto(int s, char* buf, int bufLen, int flags, struct sockaddr* to, int tolen)
{
    VxWorks64NetFake_SendtoCount++;
    VxWorks64NetFake_SendtoFd = s;
    VxWorks64NetFake_SendtoBuf = buf;
    VxWorks64NetFake_SendtoLen = bufLen;
    size_t copied = ((size_t) bufLen < sizeof(VxWorks64NetFake_SendtoPayload))
                        ? (size_t) bufLen
                        : sizeof(VxWorks64NetFake_SendtoPayload) - 1U;
    (void) memcpy(VxWorks64NetFake_SendtoPayload, buf, copied);
    VxWorks64NetFake_SendtoPayload[copied] = '\0';
    VxWorks64NetFake_SendtoFlags = flags;
    VxWorks64NetFake_SendtoTo = to;
    VxWorks64NetFake_SendtoToLen = tolen;
    int result = bufLen;
    if (VxWorks64NetFake_SendtoErrno != 0)
    {
        errno = VxWorks64NetFake_SendtoErrno;
        result = ERROR;
    }
    return result;
}

void VxWorks64NetFake_FailConnectWithErrno(int errnoValue)
{
    VxWorks64NetFake_ConnectErrno = errnoValue;
}

unsigned VxWorks64NetFake_ConnectWithTimeoutCallCount(void)
{
    return VxWorks64NetFake_ConnectCount;
}

int VxWorks64NetFake_LastConnectFd(void)
{
    return VxWorks64NetFake_ConnectFd;
}

const struct sockaddr* VxWorks64NetFake_LastConnectAddress(void)
{
    return VxWorks64NetFake_ConnectAddress;
}

int VxWorks64NetFake_LastConnectAddressLength(void)
{
    return VxWorks64NetFake_ConnectAddressLength;
}

bool VxWorks64NetFake_LastConnectWasBounded(void)
{
    return VxWorks64NetFake_ConnectBounded;
}

long VxWorks64NetFake_LastConnectTimeoutSeconds(void)
{
    return VxWorks64NetFake_ConnectTimeoutSeconds;
}

long VxWorks64NetFake_LastConnectTimeoutMicroseconds(void)
{
    return VxWorks64NetFake_ConnectTimeoutMicroseconds;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
STATUS connectWithTimeout(int sock, struct sockaddr* adrs, int adrsLen, struct timeval* timeVal)
{
    VxWorks64NetFake_ConnectCount++;
    VxWorks64NetFake_ConnectFd = sock;
    VxWorks64NetFake_ConnectAddress = adrs;
    VxWorks64NetFake_ConnectAddressLength = adrsLen;
    VxWorks64NetFake_ConnectBounded = (timeVal != NULL);
    if (timeVal != NULL)
    {
        VxWorks64NetFake_ConnectTimeoutSeconds = timeVal->tv_sec;
        VxWorks64NetFake_ConnectTimeoutMicroseconds = timeVal->tv_usec;
    }
    STATUS result = OK;
    if (VxWorks64NetFake_ConnectErrno != 0)
    {
        errno = VxWorks64NetFake_ConnectErrno;
        result = ERROR;
    }
    return result;
}

void VxWorks64NetFake_RefuseSocketOption(int level, int optname)
{
    VxWorks64NetFake_RefusedLevel = level;
    VxWorks64NetFake_RefusedOptname = optname;
}

unsigned VxWorks64NetFake_SetsockoptCallCount(void)
{
    return VxWorks64NetFake_SetsockoptCount;
}

bool VxWorks64NetFake_SocketOptionWasSetTo(int level, int optname, int value)
{
    bool found = false;
    unsigned recorded = (VxWorks64NetFake_SetsockoptCount < (unsigned) VXWORKS64NETFAKE_MAX_SOCKET_OPTIONS)
                            ? VxWorks64NetFake_SetsockoptCount
                            : (unsigned) VXWORKS64NETFAKE_MAX_SOCKET_OPTIONS;
    for (unsigned index = 0U; index < recorded; index++)
    {
        const struct VxWorks64NetFake_SocketOption* option = &VxWorks64NetFake_SocketOptions[index];
        if ((option->Fd == VxWorks64NetFake_Fd) && (option->Level == level) && (option->Optname == optname) &&
            (option->Value == value) && (option->Optlen == (int) sizeof(int)))
        {
            found = true;
        }
    }
    return found;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
STATUS setsockopt(int s, int level, int optname, char* optval, int optlen)
{
    if (VxWorks64NetFake_SetsockoptCount < (unsigned) VXWORKS64NETFAKE_MAX_SOCKET_OPTIONS)
    {
        struct VxWorks64NetFake_SocketOption* option =
            &VxWorks64NetFake_SocketOptions[VxWorks64NetFake_SetsockoptCount];
        option->Fd = s;
        option->Level = level;
        option->Optname = optname;
        option->Optlen = optlen;
        option->Value = 0;
        if (optlen == (int) sizeof(int))
        {
            (void) memcpy(&option->Value, optval, sizeof(int));
        }
    }
    VxWorks64NetFake_SetsockoptCount++;
    bool refused = (level == VxWorks64NetFake_RefusedLevel) && (optname == VxWorks64NetFake_RefusedOptname);
    return refused ? ERROR : OK;
}

void VxWorks64NetFake_FailSendWithErrno(int errnoValue)
{
    VxWorks64NetFake_SendErrno = errnoValue;
}

void VxWorks64NetFake_LimitSendTo(int bytes)
{
    VxWorks64NetFake_SendLimit = bytes;
}

unsigned VxWorks64NetFake_SendCallCount(void)
{
    return VxWorks64NetFake_SendCount;
}

int VxWorks64NetFake_LastSendFd(void)
{
    return VxWorks64NetFake_SendFd;
}

const char* VxWorks64NetFake_LastSendBuf(void)
{
    return VxWorks64NetFake_SendBuf;
}

int VxWorks64NetFake_LastSendLen(void)
{
    return VxWorks64NetFake_SendLen;
}

int VxWorks64NetFake_LastSendFlags(void)
{
    return VxWorks64NetFake_SendFlags;
}

int send(int s, const char* buf, int bufLen, int flags)
{
    VxWorks64NetFake_SendCount++;
    VxWorks64NetFake_SendFd = s;
    VxWorks64NetFake_SendBuf = buf;
    VxWorks64NetFake_SendLen = bufLen;
    VxWorks64NetFake_SendFlags = flags;
    int result = bufLen;
    if (VxWorks64NetFake_SendErrno != 0)
    {
        errno = VxWorks64NetFake_SendErrno;
        result = ERROR;
    }
    else if ((VxWorks64NetFake_SendLimit >= 0) && (VxWorks64NetFake_SendLimit < bufLen))
    {
        result = VxWorks64NetFake_SendLimit;
    }
    else
    {
        /* the whole buffer is taken */
    }
    return result;
}

void VxWorks64NetFake_FailRecvWithErrno(int errnoValue)
{
    VxWorks64NetFake_RecvErrno = errnoValue;
}

void VxWorks64NetFake_RecvDelivers(const char* data, int length)
{
    VxWorks64NetFake_RecvErrno = 0;
    VxWorks64NetFake_RecvData = data;
    VxWorks64NetFake_RecvDataLength = length;
}

unsigned VxWorks64NetFake_RecvCallCount(void)
{
    return VxWorks64NetFake_RecvCount;
}

int VxWorks64NetFake_LastRecvFd(void)
{
    return VxWorks64NetFake_RecvFd;
}

const char* VxWorks64NetFake_LastRecvBuf(void)
{
    return VxWorks64NetFake_RecvBuf;
}

int VxWorks64NetFake_LastRecvLen(void)
{
    return VxWorks64NetFake_RecvLen;
}

int VxWorks64NetFake_LastRecvFlags(void)
{
    return VxWorks64NetFake_RecvFlags;
}

int recv(int s, char* buf, int bufLen, int flags)
{
    VxWorks64NetFake_RecvCount++;
    VxWorks64NetFake_RecvFd = s;
    VxWorks64NetFake_RecvBuf = buf;
    VxWorks64NetFake_RecvLen = bufLen;
    VxWorks64NetFake_RecvFlags = flags;
    int result = ERROR;
    if (VxWorks64NetFake_RecvErrno != 0)
    {
        errno = VxWorks64NetFake_RecvErrno;
    }
    else
    {
        result = (VxWorks64NetFake_RecvDataLength < bufLen) ? VxWorks64NetFake_RecvDataLength : bufLen;
        if (result > 0)
        {
            (void) memcpy(buf, VxWorks64NetFake_RecvData, (size_t) result);
        }
    }
    return result;
}
