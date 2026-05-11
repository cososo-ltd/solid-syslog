// NOLINTBEGIN(performance-no-int-to-ptr) -- FREERTOS_INVALID_SOCKET is ((Socket_t)~0U) from FreeRTOS-Plus-TCP; the int-to-ptr cast is intrinsic to the upstream
// sentinel and unavoidable.

#include "SolidSyslogFreeRtosTcpStream.h"

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

typedef struct SolidSyslogFreeRtosTcpStream FreeRtosTcpStream;

struct SolidSyslogFreeRtosTcpStream
{
    struct SolidSyslogStream base;
    Socket_t                 socket;
};

/* FreeRTOS-Plus-TCP does not expose non-blocking connect with select(), so we
 * bound the blocking connect call with SO_SNDTIMEO instead. 200 ms is short
 * enough that the Service task keeps draining predictably during an outage,
 * long enough for a healthy peer to ACK over slirp/LAN. After connect both
 * timeouts go back to 0 so subsequent Send/Read follow the non-blocking
 * single-call contract from SolidSyslogStream. */
static const TickType_t CONNECT_TIMEOUT_TICKS = pdMS_TO_TICKS(200);
static const TickType_t NO_TIMEOUT_TICKS      = 0;

static bool                      Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool                      Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize          Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void                      Close(struct SolidSyslogStream* self);
static inline FreeRtosTcpStream* FreeRtosTcpStream_From(struct SolidSyslogStream* self);
static inline bool               IsOpen(const FreeRtosTcpStream* stream);
static void                      SetSendTimeout(Socket_t socket, TickType_t ticks);
static void                      SetRecvTimeout(Socket_t socket, TickType_t ticks);
static void                      CloseSocket(FreeRtosTcpStream* stream);

SOLIDSYSLOG_STATIC_ASSERT(sizeof(FreeRtosTcpStream) <= SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE,
                          "SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE is too small for SolidSyslogFreeRtosTcpStream layout");

static const FreeRtosTcpStream DEFAULT_INSTANCE = {
    {Open, Send, Read, Close},
    FREERTOS_INVALID_SOCKET,
};

static const FreeRtosTcpStream DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL},
    FREERTOS_INVALID_SOCKET,
};

struct SolidSyslogStream* SolidSyslogFreeRtosTcpStream_Create(SolidSyslogFreeRtosTcpStreamStorage* storage)
{
    FreeRtosTcpStream* stream = (FreeRtosTcpStream*) storage;
    *stream                   = DEFAULT_INSTANCE;
    return &stream->base;
}

void SolidSyslogFreeRtosTcpStream_Destroy(struct SolidSyslogStream* stream)
{
    FreeRtosTcpStream* self = FreeRtosTcpStream_From(stream);
    CloseSocket(self);
    *self = DESTROYED_INSTANCE;
}

static inline FreeRtosTcpStream* FreeRtosTcpStream_From(struct SolidSyslogStream* self)
{
    return (FreeRtosTcpStream*) self;
}

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    FreeRtosTcpStream* stream = FreeRtosTcpStream_From(self);
    if (!IsOpen(stream))
    {
        stream->socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        if (IsOpen(stream))
        {
            const struct freertos_sockaddr* dest = SolidSyslogAddress_AsConstFreertosSockaddr(addr);
            SetSendTimeout(stream->socket, CONNECT_TIMEOUT_TICKS);
            if (FreeRTOS_connect(stream->socket, dest, sizeof(*dest)) == 0)
            {
                SetSendTimeout(stream->socket, NO_TIMEOUT_TICKS);
                SetRecvTimeout(stream->socket, NO_TIMEOUT_TICKS);
            }
            else
            {
                CloseSocket(stream);
            }
        }
    }
    return IsOpen(stream);
}

static inline bool IsOpen(const FreeRtosTcpStream* stream)
{
    return stream->socket != FREERTOS_INVALID_SOCKET;
}

static void SetSendTimeout(Socket_t socket, TickType_t ticks)
{
    (void) FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_SNDTIMEO, &ticks, sizeof(ticks));
}

static void SetRecvTimeout(Socket_t socket, TickType_t ticks)
{
    (void) FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_RCVTIMEO, &ticks, sizeof(ticks));
}

static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    FreeRtosTcpStream* stream = FreeRtosTcpStream_From(self);
    bool               sent   = false;
    if (IsOpen(stream))
    {
        BaseType_t rc = FreeRTOS_send(stream->socket, buffer, size, 0);
        sent          = (rc >= 0) && ((size_t) rc == size);
        if (!sent)
        {
            CloseSocket(stream);
        }
    }
    return sent;
}

/* FreeRTOS_recv with RCVTIMEO=0 returns 0 when no data is available (would
 * block); the Service thread treats that as "nothing to read right now".
 * Negative returns are real errors — close the socket and surface failure. */
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    FreeRtosTcpStream* stream = FreeRtosTcpStream_From(self);
    SolidSyslogSsize   result = -1;
    if (IsOpen(stream))
    {
        BaseType_t rc = FreeRTOS_recv(stream->socket, buffer, size, 0);
        if (rc >= 0)
        {
            result = (SolidSyslogSsize) rc;
        }
        else
        {
            CloseSocket(stream);
        }
    }
    return result;
}

static void Close(struct SolidSyslogStream* self)
{
    CloseSocket(FreeRtosTcpStream_From(self));
}

static void CloseSocket(FreeRtosTcpStream* stream)
{
    if (IsOpen(stream))
    {
        (void) FreeRTOS_closesocket(stream->socket);
        stream->socket = FREERTOS_INVALID_SOCKET;
    }
}

// NOLINTEND(performance-no-int-to-ptr)
