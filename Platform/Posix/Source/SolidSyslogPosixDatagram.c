#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogUdpPayload.h"

#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

enum
{
    INVALID_FD = -1
};

static bool                               Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr);
static size_t                             MaxPayload(struct SolidSyslogDatagram* self);
static void                               Close(struct SolidSyslogDatagram* self);
static inline bool                        IsFileDescriptorValid(int fd);

struct SolidSyslogPosixDatagram
{
    struct SolidSyslogDatagram base;
    int                        fd;
};

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
    return IsFileDescriptorValid(datagram->fd);
}

static inline bool IsFileDescriptorValid(int fd)
{
    return fd >= 0;
}

static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    const struct sockaddr_in*        sin      = SolidSyslogAddress_AsConstSockaddrIn(addr);
    ssize_t                          sent     = sendto(datagram->fd, buffer, size, 0, (const struct sockaddr*) sin, sizeof(*sin));
    return (sent >= 0) ? SOLIDSYSLOG_DATAGRAM_SENT : SOLIDSYSLOG_DATAGRAM_FAILED;
}

static size_t MaxPayload(struct SolidSyslogDatagram* self)
{
    (void) self;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}

static void Close(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogPosixDatagram* datagram = (struct SolidSyslogPosixDatagram*) self;
    if (IsFileDescriptorValid(datagram->fd))
    {
        close(datagram->fd);
        datagram->fd = INVALID_FD;
    }
}
