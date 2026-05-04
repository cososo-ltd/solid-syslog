#include "SolidSyslogPosixDatagram.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogDatagram.h"

struct SolidSyslogAddress;

enum
{
    INVALID_FD = -1
};

struct SolidSyslogPosixDatagram
{
    struct SolidSyslogDatagram base;
    int                        fd;
    bool                       connected;
};

static bool                               Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr);
static size_t                             MaxPayload(struct SolidSyslogDatagram* self);
static void                               Close(struct SolidSyslogDatagram* self);
static inline bool                        ConnectIfNeeded(struct SolidSyslogPosixDatagram* datagram, const struct SolidSyslogAddress* addr);
static inline bool                        IsFileDescriptorValid(int fd);

static struct SolidSyslogPosixDatagram instance = {.fd = INVALID_FD};

struct SolidSyslogDatagram* SolidSyslogPosixDatagram_Create(void)
{
    instance.base.Open       = Open;
    instance.base.SendTo     = SendTo;
    instance.base.MaxPayload = MaxPayload;
    instance.base.Close      = Close;
    return &instance.base;
}

void SolidSyslogPosixDatagram_Destroy(void)
{
    instance.base.Open       = NULL;
    instance.base.SendTo     = NULL;
    instance.base.MaxPayload = NULL;
    instance.base.Close      = NULL;
}

static bool Open(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    datagram->fd                              = socket(AF_INET, SOCK_DGRAM, 0);
    datagram->connected                       = false;
    return IsFileDescriptorValid(datagram->fd);
}

static inline bool IsFileDescriptorValid(int fd)
{
    return fd >= 0;
}

static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogPosixDatagram*   datagram = (struct SolidSyslogPosixDatagram*) self;
    enum SolidSyslogDatagramSendResult result   = SOLIDSYSLOG_DATAGRAM_FAILED;
    if (ConnectIfNeeded(datagram, addr))
    {
        const struct sockaddr_in* sin  = SolidSyslogAddress_AsConstSockaddrIn(addr);
        ssize_t                   sent = sendto(datagram->fd, buffer, size, 0, (const struct sockaddr*) sin, sizeof(*sin));
        if (sent >= 0)
        {
            result = SOLIDSYSLOG_DATAGRAM_SENT;
        }
        else if (errno == EMSGSIZE)
        {
            result = SOLIDSYSLOG_DATAGRAM_OVERSIZE;
        }
    }
    return result;
}

static inline bool ConnectIfNeeded(struct SolidSyslogPosixDatagram* datagram, const struct SolidSyslogAddress* addr)
{
    if (!datagram->connected)
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        if (connect(datagram->fd, (const struct sockaddr*) sin, sizeof(*sin)) == 0)
        {
            const int pmtu = IP_PMTUDISC_DO;
            (void) setsockopt(datagram->fd, IPPROTO_IP, IP_MTU_DISCOVER, &pmtu, sizeof(pmtu));
            datagram->connected = true;
        }
    }
    return datagram->connected;
}

static size_t MaxPayload(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    size_t                           result   = SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
    if (datagram->connected)
    {
        int       mtu    = 0;
        socklen_t optlen = sizeof(mtu);
        if ((getsockopt(datagram->fd, IPPROTO_IP, IP_MTU, &mtu, &optlen) == 0) && (mtu > 0))
        {
            result = SolidSyslogUdpPayload_FromMtu((size_t) mtu, false);
        }
    }
    return result;
}

static void Close(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    if (IsFileDescriptorValid(datagram->fd))
    {
        close(datagram->fd);
        datagram->fd        = INVALID_FD;
        datagram->connected = false;
    }
}
