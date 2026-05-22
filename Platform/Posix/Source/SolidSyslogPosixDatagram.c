#include "SolidSyslogPosixDatagram.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPosixDatagramPrivate.h"
#include "SolidSyslogUdpPayload.h"

struct SolidSyslogAddress;

enum
{
    INVALID_FD = -1
};

static bool PosixDatagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult PosixDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t PosixDatagram_MaxPayload(struct SolidSyslogDatagram* base);
static void PosixDatagram_Close(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogPosixDatagram* PosixDatagram_SelfFromBase(struct SolidSyslogDatagram* base);
static inline bool PosixDatagram_ConnectIfNeeded(
    struct SolidSyslogPosixDatagram* self,
    const struct SolidSyslogAddress* addr
);
static inline bool PosixDatagram_IsFileDescriptorValid(int fd);

void PosixDatagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPosixDatagram* self = PosixDatagram_SelfFromBase(base);
    self->Base.Open = PosixDatagram_Open;
    self->Base.SendTo = PosixDatagram_SendTo;
    self->Base.MaxPayload = PosixDatagram_MaxPayload;
    self->Base.Close = PosixDatagram_Close;
    self->Fd = INVALID_FD;
    self->Connected = false;
}

static inline struct SolidSyslogPosixDatagram* PosixDatagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogPosixDatagram*) base;
}

void PosixDatagram_Cleanup(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPosixDatagram* self = PosixDatagram_SelfFromBase(base);
    if (PosixDatagram_IsFileDescriptorValid(self->Fd))
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
        self->Connected = false;
    }
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullDatagram_Get();
}

static inline bool PosixDatagram_IsFileDescriptorValid(int fd)
{
    return fd >= 0;
}

static bool PosixDatagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPosixDatagram* self = PosixDatagram_SelfFromBase(base);
    self->Fd = socket(AF_INET, SOCK_DGRAM, 0);
    self->Connected = false;
    return PosixDatagram_IsFileDescriptorValid(self->Fd);
}

static enum SolidSyslogDatagramSendResult PosixDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogPosixDatagram* self = PosixDatagram_SelfFromBase(base);
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
    if (PosixDatagram_ConnectIfNeeded(self, addr))
    {
        const struct sockaddr_in* sin = SolidSyslogPosixAddress_AsConstSockaddrIn(addr);
        ssize_t sent = sendto(self->Fd, buffer, size, 0, (const struct sockaddr*) sin, sizeof(*sin));
        if (sent >= 0)
        {
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
        }
        else if (errno == EMSGSIZE)
        {
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE;
        }
        else
        {
            /* generic send failure — result stays Failed */
        }
    }
    return result;
}

static inline bool PosixDatagram_ConnectIfNeeded(
    struct SolidSyslogPosixDatagram* self,
    const struct SolidSyslogAddress* addr
)
{
    if (!self->Connected)
    {
        const struct sockaddr_in* sin = SolidSyslogPosixAddress_AsConstSockaddrIn(addr);
        if (connect(self->Fd, (const struct sockaddr*) sin, sizeof(*sin)) == 0)
        {
            const int pmtu = IP_PMTUDISC_DO;
            (void) setsockopt(self->Fd, IPPROTO_IP, IP_MTU_DISCOVER, &pmtu, sizeof(pmtu));
            self->Connected = true;
        }
    }
    return self->Connected;
}

static size_t PosixDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPosixDatagram* self = PosixDatagram_SelfFromBase(base);
    size_t result = SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
    if (self->Connected)
    {
        int mtu = 0;
        socklen_t optlen = sizeof(mtu);
        if ((getsockopt(self->Fd, IPPROTO_IP, IP_MTU, &mtu, &optlen) == 0) && (mtu > 0))
        {
            result = SolidSyslogUdpPayload_FromMtu((size_t) mtu, false);
        }
    }
    return result;
}

static void PosixDatagram_Close(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPosixDatagram* self = PosixDatagram_SelfFromBase(base);
    if (PosixDatagram_IsFileDescriptorValid(self->Fd))
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
        self->Connected = false;
    }
}
