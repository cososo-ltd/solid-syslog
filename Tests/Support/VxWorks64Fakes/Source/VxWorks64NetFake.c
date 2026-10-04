#include "vxWorks.h"

#include "VxWorks64NetFake.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>

#include "hostLib.h"
#include "ioLib.h"
#include "inetLib.h"
#include "sockLib.h"

static const char* VxWorks64NetFake_Hostname = "";
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
static unsigned VxWorks64NetFake_CloseCount = 0U;
static int VxWorks64NetFake_ClosedFd = -1;

void VxWorks64NetFake_Reset(void)
{
    VxWorks64NetFake_Hostname = "";
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
    VxWorks64NetFake_CloseCount = 0U;
    VxWorks64NetFake_ClosedFd = -1;
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

int gethostname(char* name, int nameLen)
{
    size_t length = strlen(VxWorks64NetFake_Hostname) + 1U;
    size_t copied = (length < (size_t) nameLen) ? length : (size_t) nameLen;
    (void) memcpy(name, VxWorks64NetFake_Hostname, copied);
    return OK;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
int sethostname(char* name, int nameLen)
{
    (void) name;
    (void) nameLen;
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

unsigned VxWorks64NetFake_CloseCallCount(void)
{
    return VxWorks64NetFake_CloseCount;
}

int VxWorks64NetFake_LastClosedFd(void)
{
    return VxWorks64NetFake_ClosedFd;
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

STATUS close(int fd)
{
    VxWorks64NetFake_CloseCount++;
    VxWorks64NetFake_ClosedFd = fd;
    return OK;
}
