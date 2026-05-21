#include "SocketStream.h"

#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogAddress;

struct SocketStream
{
    struct SolidSyslogStream Base;
    int Fd;
};

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void Close(struct SolidSyslogStream* self);

struct SolidSyslogStream* SocketStream_Create(int fd)
{
    struct SocketStream* stream = (struct SocketStream*) calloc(1, sizeof(struct SocketStream));
    stream->Base.Open = Open;
    stream->Base.Send = Send;
    stream->Base.Read = Read;
    stream->Base.Close = Close;
    stream->Fd = fd;
    return &stream->Base;
}

void SocketStream_Destroy(struct SolidSyslogStream* self)
{
    struct SocketStream* stream = (struct SocketStream*) self;
    /* Idempotent close: if the production Close path already ran it will
     * have set Fd to -1. Otherwise (e.g. handshake failure where Close
     * never runs) we still own the fd and must release it so the server's
     * blocking recv unblocks promptly. */
    if (stream->Fd >= 0)
    {
        close(stream->Fd);
        stream->Fd = -1;
    }
    free(self);
}

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    /* The fd is pre-connected (e.g. socketpair); Open is a no-op so the test
     * harness fully controls connection lifecycle. */
    (void) self;
    (void) addr;
    return true;
}

static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct SocketStream* stream = (struct SocketStream*) self;
    const unsigned char* bytes = (const unsigned char*) buffer;
    size_t remaining = size;
    while (remaining > 0)
    {
        ssize_t n = send(stream->Fd, bytes, remaining, 0);
        if (n <= 0)
        {
            return false;
        }
        bytes += n;
        remaining -= (size_t) n;
    }
    return true;
}

static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    struct SocketStream* stream = (struct SocketStream*) self;
    ssize_t n = recv(stream->Fd, buffer, size, 0);
    return (SolidSyslogSsize) n;
}

static void Close(struct SolidSyslogStream* self)
{
    struct SocketStream* stream = (struct SocketStream*) self;
    if (stream->Fd >= 0)
    {
        close(stream->Fd);
        stream->Fd = -1;
    }
}
