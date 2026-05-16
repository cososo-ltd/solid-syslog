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
    struct SolidSyslogDatagram Base;
    int Fd;
    bool Connected;
};

static bool PosixDatagram_Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult PosixDatagram_SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t PosixDatagram_MaxPayload(struct SolidSyslogDatagram* self);
static void PosixDatagram_Close(struct SolidSyslogDatagram* self);
static inline bool PosixDatagram_ConnectIfNeeded(
    struct SolidSyslogPosixDatagram* datagram,
    const struct SolidSyslogAddress* addr
);
static inline bool PosixDatagram_IsFileDescriptorValid(int fd);

static struct SolidSyslogPosixDatagram instance = {.Fd = INVALID_FD};

struct SolidSyslogDatagram* SolidSyslogPosixDatagram_Create(void)
{
    instance.Base.Open = PosixDatagram_Open;
    instance.Base.SendTo = PosixDatagram_SendTo;
    instance.Base.MaxPayload = PosixDatagram_MaxPayload;
    instance.Base.Close = PosixDatagram_Close;
    return &instance.Base;
}

void SolidSyslogPosixDatagram_Destroy(void)
{
    instance.Base.Open = NULL;
    instance.Base.SendTo = NULL;
    instance.Base.MaxPayload = NULL;
    instance.Base.Close = NULL;
}

static bool PosixDatagram_Open(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    datagram->Fd = socket(AF_INET, SOCK_DGRAM, 0);
    datagram->Connected = false;
    return PosixDatagram_IsFileDescriptorValid(datagram->Fd);
}

static inline bool PosixDatagram_IsFileDescriptorValid(int fd)
{
    return fd >= 0;
}

static enum SolidSyslogDatagramSendResult PosixDatagram_SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagramSendResult_Failed;
    if (PosixDatagram_ConnectIfNeeded(datagram, addr))
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        ssize_t sent = sendto(datagram->Fd, buffer, size, 0, (const struct sockaddr*) sin, sizeof(*sin));
        if (sent >= 0)
        {
            result = SolidSyslogDatagramSendResult_Sent;
        }
        else if (errno == EMSGSIZE)
        {
            result = SolidSyslogDatagramSendResult_Oversize;
        }
        else
        {
            /* generic send failure — result stays Failed */
        }
    }
    return result;
}

static inline bool PosixDatagram_ConnectIfNeeded(
    struct SolidSyslogPosixDatagram* datagram,
    const struct SolidSyslogAddress* addr
)
{
    if (!datagram->Connected)
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        if (connect(datagram->Fd, (const struct sockaddr*) sin, sizeof(*sin)) == 0)
        {
            const int pmtu = IP_PMTUDISC_DO;
            (void) setsockopt(datagram->Fd, IPPROTO_IP, IP_MTU_DISCOVER, &pmtu, sizeof(pmtu));
            datagram->Connected = true;
        }
    }
    return datagram->Connected;
}

static size_t PosixDatagram_MaxPayload(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    size_t result = SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
    if (datagram->Connected)
    {
        int mtu = 0;
        socklen_t optlen = sizeof(mtu);
        if ((getsockopt(datagram->Fd, IPPROTO_IP, IP_MTU, &mtu, &optlen) == 0) && (mtu > 0))
        {
            result = SolidSyslogUdpPayload_FromMtu((size_t) mtu, false);
        }
    }
    return result;
}

static void PosixDatagram_Close(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    if (PosixDatagram_IsFileDescriptorValid(datagram->Fd))
    {
        close(datagram->Fd);
        datagram->Fd = INVALID_FD;
        datagram->Connected = false;
    }
}
