#include "SocketStream.h"

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>

#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogAddress;

struct SocketStream
{
    struct SolidSyslogStream Base;
    int Fd;
};

static bool SocketStream_Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool SocketStream_Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize SocketStream_Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void SocketStream_Close(struct SolidSyslogStream* self);
static uint32_t SocketStream_Version(struct SolidSyslogStream* self);
static bool SocketStream_WouldBlock(const struct SocketStream* stream);

struct SolidSyslogStream* SocketStream_Create(int fd)
{
    struct SocketStream* stream = (struct SocketStream*) calloc(1, sizeof(struct SocketStream));
    stream->Base.Open = SocketStream_Open;
    stream->Base.Send = SocketStream_Send;
    stream->Base.Read = SocketStream_Read;
    stream->Base.Close = SocketStream_Close;
    stream->Base.Version = SocketStream_Version;
    stream->Fd = fd;
    return &stream->Base;
}

void SocketStream_Destroy(struct SolidSyslogStream* self)
{
    /* Idempotent: SocketStream_Close handles the fd lifecycle and sets Fd
     * to -1 so this stays safe whether production Close ran first or not. */
    SocketStream_Close(self);
    free(self);
}

static bool SocketStream_Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    /* The fd is pre-connected (e.g. socketpair); Open is a no-op so the test
     * harness fully controls connection lifecycle. */
    (void) self;
    (void) addr;
    return true;
}

static bool SocketStream_Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct SocketStream* stream = (struct SocketStream*) self;
    const unsigned char* bytes = (const unsigned char*) buffer;
    size_t remaining = size;
    while (remaining > 0)
    {
        ssize_t n = send(stream->Fd, bytes, remaining, 0);
        if (n <= 0)
        {
            /* The whole buffer could not go, so the Stream contract has this
             * close internally before failing - the caller reopens and
             * store-and-forward replays. */
            SocketStream_Close(self);
            return false;
        }
        bytes += n;
        remaining -= (size_t) n;
    }
    return true;
}

static SolidSyslogSsize SocketStream_Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    /* Distinguish EOF (peer closed: recv == 0) from would-block (the Stream
     * contract reserves 0 for "no data available, try again"). A TLS stream's
     * receive callback maps a transport 0 to the library's want-read -
     * forwarding a real EOF as 0 would loop the handshake until the budget
     * exhausts. Map EOF to -1 (fatal) instead. */
    struct SocketStream* stream = (struct SocketStream*) self;
    ssize_t n = recv(stream->Fd, buffer, size, 0);
    SolidSyslogSsize result = (SolidSyslogSsize) n;
    if (n == 0)
    {
        result = -1;
    }
    else if ((n < 0) && SocketStream_WouldBlock(stream))
    {
        /* Only a non-blocking descriptor answers would-block as 0. A blocking
         * one that ran out its receive timeout has the same errno, and that is
         * the suite's protection against a handshake that hangs, so it stays a
         * teardown. */
        result = 0;
    }
    else
    {
        /* A count, or a failure that the check below closes on. */
    }
    if (result < 0)
    {
        /* A negative return is reserved for a real teardown, and the contract
         * has the stream closed before one is made. */
        SocketStream_Close(self);
    }
    return result;
}

static void SocketStream_Close(struct SolidSyslogStream* self)
{
    struct SocketStream* stream = (struct SocketStream*) self;
    if (stream->Fd >= 0)
    {
        close(stream->Fd);
        stream->Fd = -1;
    }
}

static uint32_t SocketStream_Version(struct SolidSyslogStream* self)
{
    (void) self;
    return 0U;
}

static bool SocketStream_WouldBlock(const struct SocketStream* stream)
{
    return ((errno == EAGAIN) || (errno == EWOULDBLOCK)) && ((fcntl(stream->Fd, F_GETFL) & O_NONBLOCK) != 0);
}
