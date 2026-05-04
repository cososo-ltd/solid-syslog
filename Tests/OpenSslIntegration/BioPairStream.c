#include "BioPairStream.h"

#include <openssl/bio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogStream.h"

struct SolidSyslogAddress;

struct BioPairStream
{
    struct SolidSyslogStream  base;
    BIO*                      bio;
    BioPairStreamPumpFunction pump;
    void*                     pumpContext;
};

static bool             Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool             Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void             Close(struct SolidSyslogStream* self);

struct SolidSyslogStream* BioPairStream_Create(BIO* bio)
{
    struct BioPairStream* stream = (struct BioPairStream*) calloc(1, sizeof(struct BioPairStream));
    stream->base.Open            = Open;
    stream->base.Send            = Send;
    stream->base.Read            = Read;
    stream->base.Close           = Close;
    stream->bio                  = bio;
    return &stream->base;
}

void BioPairStream_Destroy(struct SolidSyslogStream* self)
{
    free(self);
}

void BioPairStream_SetPump(struct SolidSyslogStream* self, BioPairStreamPumpFunction pump, void* context)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    stream->pump                 = pump;
    stream->pumpContext          = context;
}

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    (void) self;
    (void) addr;
    return true;
}

static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct BioPairStream* stream  = (struct BioPairStream*) self;
    int                   written = BIO_write(stream->bio, buffer, (int) size);
    return written == (int) size;
}

/* Reads from the BIO, driving the paired peer via the pump callback whenever the
 * read would otherwise block. Models a blocking stream on top of a non-blocking
 * BIO pair so SolidSyslogTlsStream's synchronous SSL_connect / SSL_read path
 * works unchanged. */
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    struct BioPairStream* stream = (struct BioPairStream*) self;
    SolidSyslogSsize      result = -1;
    bool                  done   = false;
    while (!done)
    {
        int bytesRead = BIO_read(stream->bio, buffer, (int) size);
        if (bytesRead > 0)
        {
            result = (SolidSyslogSsize) bytesRead;
            done   = true;
        }
        else if (!BIO_should_retry(stream->bio) || stream->pump == NULL)
        {
            done = true;
        }
        else
        {
            stream->pump(stream->pumpContext);
        }
    }
    return result;
}

static void Close(struct SolidSyslogStream* self)
{
    (void) self;
}
