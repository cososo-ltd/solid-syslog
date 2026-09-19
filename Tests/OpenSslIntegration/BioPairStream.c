#include "BioPairStream.h"

#include <openssl/bio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogStream.h"

struct SolidSyslogAddress;

struct BioPairStream
{
    struct SolidSyslogStream Base;
    BIO* Bio;
    BioPairStreamPumpFunction Pump;
    void* PumpContext;
    /* The Stream contract has an implementation close internally on a failed
     * Send and before a negative Read, so the caller reopens and
     * store-and-forward replays. Closing cannot free the BIO here - the
     * pair's lifetime belongs to the harness, which goes on pumping the peer
     * - so it is recorded instead, and the stream carries nothing until it is
     * reopened. That is the part of the clause a caller can observe. */
    bool Closed;
};

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void Close(struct SolidSyslogStream* self);
static uint32_t Version(struct SolidSyslogStream* self);

struct SolidSyslogStream* BioPairStream_Create(BIO* bio)
{
    struct BioPairStream* stream = (struct BioPairStream*) calloc(1, sizeof(struct BioPairStream));
    stream->Base.Open = Open;
    stream->Base.Send = Send;
    stream->Base.Read = Read;
    stream->Base.Close = Close;
    stream->Base.Version = Version;
    stream->Bio = bio;
    return &stream->Base;
}

void BioPairStream_Destroy(struct SolidSyslogStream* self)
{
    free(self);
}

void BioPairStream_SetPump(struct SolidSyslogStream* self, BioPairStreamPumpFunction pump, void* context)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    stream->Pump = pump;
    stream->PumpContext = context;
}

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    (void) addr;
    stream->Closed = false;
    return true;
}

static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    bool sent = false;
    if (!stream->Closed)
    {
        int written = BIO_write(stream->Bio, buffer, (int) size);
        sent = written == (int) size;
        if (!sent)
        {
            Close(self);
        }
    }
    return sent;
}

/* Reads from the BIO, driving the paired peer via the pump callback whenever the
 * read would otherwise block. Models a blocking stream on top of a non-blocking
 * BIO pair so SolidSyslogOpenSslStream's synchronous SSL_connect / SSL_read path
 * works unchanged. */
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    SolidSyslogSsize result = -1;
    bool done = stream->Closed;
    while (!done)
    {
        int bytesRead = BIO_read(stream->Bio, buffer, (int) size);
        if (bytesRead > 0)
        {
            result = (SolidSyslogSsize) bytesRead;
            done = true;
        }
        else if (!BIO_should_retry(stream->Bio) || stream->Pump == NULL)
        {
            /* Giving up means returning a negative, which the contract calls
             * a teardown and has the stream closed before. */
            Close(self);
            done = true;
        }
        else
        {
            stream->Pump(stream->PumpContext);
        }
    }
    return result;
}

static void Close(struct SolidSyslogStream* self)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    stream->Closed = true;
}

static uint32_t Version(struct SolidSyslogStream* self)
{
    (void) self;
    return 0U;
}
