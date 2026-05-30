#include "ReconnectingSocketStream.h"

#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogAddress;

enum
{
    RECONNECTING_SOCKET_STREAM_MAX_FDS = 4
};

struct ReconnectingSocketStream
{
    struct SolidSyslogStream Base;
    int Fds[RECONNECTING_SOCKET_STREAM_MAX_FDS];
    int Count;
    int NextIndex; /* index of the fd the next Open will consume */
    int CurrentFd; /* fd handed out by the most recent Open; -1 when closed */
};

static bool ReconnectingSocketStream_Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool ReconnectingSocketStream_Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize ReconnectingSocketStream_Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void ReconnectingSocketStream_Close(struct SolidSyslogStream* self);

struct SolidSyslogStream* ReconnectingSocketStream_Create(const int* fds, int count)
{
    struct ReconnectingSocketStream* stream =
        (struct ReconnectingSocketStream*) calloc(1, sizeof(struct ReconnectingSocketStream));
    stream->Base.Open = ReconnectingSocketStream_Open;
    stream->Base.Send = ReconnectingSocketStream_Send;
    stream->Base.Read = ReconnectingSocketStream_Read;
    stream->Base.Close = ReconnectingSocketStream_Close;
    stream->Count = (count < RECONNECTING_SOCKET_STREAM_MAX_FDS) ? count : RECONNECTING_SOCKET_STREAM_MAX_FDS;
    for (int i = 0; i < stream->Count; i++)
    {
        stream->Fds[i] = fds[i];
    }
    stream->NextIndex = 0;
    stream->CurrentFd = -1;
    return &stream->Base;
}

void ReconnectingSocketStream_Destroy(struct SolidSyslogStream* self)
{
    struct ReconnectingSocketStream* stream = (struct ReconnectingSocketStream*) self;
    ReconnectingSocketStream_Close(self);
    /* Close any fds the test never consumed (e.g. a test that failed before
     * the second Open) so the harness leaks no descriptors. */
    for (int i = stream->NextIndex; i < stream->Count; i++)
    {
        if (stream->Fds[i] >= 0)
        {
            close(stream->Fds[i]);
            stream->Fds[i] = -1;
        }
    }
    free(stream);
}

static bool ReconnectingSocketStream_Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    (void) addr;
    struct ReconnectingSocketStream* stream = (struct ReconnectingSocketStream*) self;
    bool ok = false;
    if (stream->NextIndex < stream->Count)
    {
        stream->CurrentFd = stream->Fds[stream->NextIndex];
        stream->NextIndex++;
        ok = true;
    }
    return ok;
}

static bool ReconnectingSocketStream_Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct ReconnectingSocketStream* stream = (struct ReconnectingSocketStream*) self;
    const unsigned char* bytes = (const unsigned char*) buffer;
    size_t remaining = size;
    bool ok = true;
    while ((remaining > 0) && ok)
    {
        ssize_t n = send(stream->CurrentFd, bytes, remaining, 0);
        if (n <= 0)
        {
            ok = false;
        }
        else
        {
            bytes += n;
            remaining -= (size_t) n;
        }
    }
    return ok;
}

static SolidSyslogSsize ReconnectingSocketStream_Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    /* Same EOF / would-block mapping as SocketStream: recv == 0 means the peer
     * closed (fatal -> -1); the Stream contract reserves 0 for would-block,
     * which a blocking socketpair never produces. */
    struct ReconnectingSocketStream* stream = (struct ReconnectingSocketStream*) self;
    ssize_t n = recv(stream->CurrentFd, buffer, size, 0);
    SolidSyslogSsize result = (SolidSyslogSsize) n;
    if (n == 0)
    {
        result = -1;
    }
    return result;
}

static void ReconnectingSocketStream_Close(struct SolidSyslogStream* self)
{
    struct ReconnectingSocketStream* stream = (struct ReconnectingSocketStream*) self;
    if (stream->CurrentFd >= 0)
    {
        close(stream->CurrentFd);
        stream->CurrentFd = -1;
    }
}
