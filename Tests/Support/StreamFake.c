#include "TestCalloc.h"
#include "StreamFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogTlsStreamErrors.h"

#include <stdlib.h>
#include <string.h>

struct StreamFake
{
    struct SolidSyslogStream Base;
    int OpenCallCount;
    const struct SolidSyslogAddress* LastOpenAddr;
    bool OpenFails;
    bool HandshakeRefused;
    bool RefusalReported; /* the run's report is made; an Open that succeeds ends the run */
    int SendCallCount;
    const void* LastSendBuf;
    size_t LastSendSize;
    bool SendFails;
    int ReadCallCount;
    void* LastReadBuf;
    size_t LastReadSize;
    SolidSyslogSsize ReadReturn;
    int CloseCallCount;
    uint32_t Version;
    /* Frames completed so far: each NUL-terminated, back to back in Frames,
     * found by its offset. */
    char* Frames;
    size_t FramesUsed;
    size_t FramesCapacity;
    size_t* FrameOffsets;
    size_t FrameCount;
    size_t FrameCapacity;
    /* The frame in progress: the length being read from its prefix, then
     * the body bytes received against that length. */
    bool PrefixComplete;
    bool PrefixGarbage; /* a byte that was not a digit: the frame is dropped, not kept */
    size_t ExpectedLength;
    char* Body;
    size_t BodyReceived;
};

static void Reassemble(struct StreamFake* fake, const uint8_t* bytes, size_t count);
static void TakePrefixByte(struct StreamFake* fake, uint8_t byte);
static void TakeBodyByte(struct StreamFake* fake, uint8_t byte);
static void CompleteFrameIfBodyIsWhole(struct StreamFake* fake);
static void KeepFrame(struct StreamFake* fake);
static void DropPartialFrame(struct StreamFake* fake);
static void Grow(void** storage, size_t* capacity, size_t needed, size_t elementSize);
static void ReportRefusalOncePerRun(struct StreamFake* fake);

const struct SolidSyslogErrorSource StreamFakeErrorSource = {"StreamFake", SolidSyslogTlsStreamErrors_Name};

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    struct StreamFake* fake = (struct StreamFake*) self;
    fake->OpenCallCount++;
    fake->LastOpenAddr = addr;
    bool opened = !fake->OpenFails && !fake->HandshakeRefused;
    if (fake->HandshakeRefused)
    {
        ReportRefusalOncePerRun(fake);
    }
    fake->RefusalReported = fake->RefusalReported && !opened;
    return opened;
}

static void ReportRefusalOncePerRun(struct StreamFake* fake)
{
    if (!fake->RefusalReported)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &StreamFakeErrorSource,
            SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
            (int32_t) SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED
        );
        fake->RefusalReported = true;
    }
}

static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct StreamFake* fake = (struct StreamFake*) self;
    fake->SendCallCount++;
    fake->LastSendBuf = buffer;
    fake->LastSendSize = size;
    if (!fake->SendFails)
    {
        Reassemble(fake, (const uint8_t*) buffer, size);
    }
    return !fake->SendFails;
}

/* The RFC 6587 framing a stream sender writes: decimal digits, a space, then
 * that many bytes, however the sender split them across Send calls. */
static void Reassemble(struct StreamFake* fake, const uint8_t* bytes, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (fake->PrefixComplete)
        {
            TakeBodyByte(fake, bytes[i]);
        }
        else
        {
            TakePrefixByte(fake, bytes[i]);
        }
    }
}

static void TakePrefixByte(struct StreamFake* fake, uint8_t byte)
{
    if (byte == ' ')
    {
        fake->PrefixComplete = true;
        /* A test fake: a failed realloc has nowhere to go but abort. */
        char* body = (char*) realloc(fake->Body, fake->ExpectedLength + 1U);
        if (body == NULL)
        {
            abort();
        }
        fake->Body = body;
        fake->BodyReceived = 0U;
        CompleteFrameIfBodyIsWhole(fake);
    }
    else if ((byte >= (uint8_t) '0') && (byte <= (uint8_t) '9'))
    {
        fake->ExpectedLength = (fake->ExpectedLength * 10U) + (size_t) (byte - (uint8_t) '0');
    }
    else
    {
        fake->PrefixGarbage = true;
    }
}

static void TakeBodyByte(struct StreamFake* fake, uint8_t byte)
{
    fake->Body[fake->BodyReceived] = (char) byte;
    fake->BodyReceived++;
    CompleteFrameIfBodyIsWhole(fake);
}

static void CompleteFrameIfBodyIsWhole(struct StreamFake* fake)
{
    if (fake->BodyReceived == fake->ExpectedLength)
    {
        if (!fake->PrefixGarbage)
        {
            KeepFrame(fake);
        }
        DropPartialFrame(fake);
    }
}

static void KeepFrame(struct StreamFake* fake)
{
    size_t bytes = fake->ExpectedLength + 1U;
    Grow((void**) &fake->Frames, &fake->FramesCapacity, fake->FramesUsed + bytes, sizeof(char));
    Grow((void**) &fake->FrameOffsets, &fake->FrameCapacity, fake->FrameCount + 1U, sizeof(size_t));
    memcpy(&fake->Frames[fake->FramesUsed], fake->Body, fake->ExpectedLength);
    fake->Frames[fake->FramesUsed + fake->ExpectedLength] = '\0';
    fake->FrameOffsets[fake->FrameCount] = fake->FramesUsed;
    fake->FramesUsed += bytes;
    fake->FrameCount++;
}

static void DropPartialFrame(struct StreamFake* fake)
{
    fake->PrefixComplete = false;
    fake->PrefixGarbage = false;
    fake->ExpectedLength = 0U;
    fake->BodyReceived = 0U;
}

static void Grow(void** storage, size_t* capacity, size_t needed, size_t elementSize)
{
    if (needed > *capacity)
    {
        size_t grown = (*capacity == 0U) ? 256U : (*capacity * 2U);
        while (grown < needed)
        {
            grown *= 2U;
        }
        void* moved = realloc(*storage, grown * elementSize);
        if (moved == NULL)
        {
            abort();
        }
        *storage = moved;
        *capacity = grown;
    }
}

static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    struct StreamFake* fake = (struct StreamFake*) self;
    fake->ReadCallCount++;
    fake->LastReadBuf = buffer;
    fake->LastReadSize = size;
    return fake->ReadReturn;
}

static uint32_t Version(struct SolidSyslogStream* self)
{
    struct StreamFake* fake = (struct StreamFake*) self;
    return fake->Version;
}

/* A frame cut by a close never reaches a collector whole: the sender
 * reconnects and writes it again from its prefix. */
static void Close(struct SolidSyslogStream* self)
{
    struct StreamFake* fake = (struct StreamFake*) self;
    fake->CloseCallCount++;
    DropPartialFrame(fake);
}

struct SolidSyslogStream* StreamFake_Create(void)
{
    struct StreamFake* fake = (struct StreamFake*) TestCalloc(sizeof(struct StreamFake));
    fake->Base.Open = Open;
    fake->Base.Send = Send;
    fake->Base.Read = Read;
    fake->Base.Close = Close;
    fake->Base.Version = Version;
    return &fake->Base;
}

void StreamFake_Destroy(struct SolidSyslogStream* stream)
{
    struct StreamFake* fake = (struct StreamFake*) stream;
    free(fake->Frames);
    free(fake->FrameOffsets);
    free(fake->Body);
    free(fake);
}

int StreamFake_OpenCallCount(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->OpenCallCount;
}

const struct SolidSyslogAddress* StreamFake_LastOpenAddr(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->LastOpenAddr;
}

int StreamFake_SendCallCount(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->SendCallCount;
}

const void* StreamFake_LastSendBuf(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->LastSendBuf;
}

size_t StreamFake_LastSendSize(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->LastSendSize;
}

int StreamFake_ReadCallCount(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->ReadCallCount;
}

void* StreamFake_LastReadBuf(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->LastReadBuf;
}

size_t StreamFake_LastReadSize(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->LastReadSize;
}

void StreamFake_SetReadReturn(struct SolidSyslogStream* stream, SolidSyslogSsize value)
{
    ((struct StreamFake*) stream)->ReadReturn = value;
}

void StreamFake_SetOpenFails(struct SolidSyslogStream* stream, bool fails)
{
    ((struct StreamFake*) stream)->OpenFails = fails;
}

void StreamFake_SetSendFails(struct SolidSyslogStream* stream, bool fails)
{
    ((struct StreamFake*) stream)->SendFails = fails;
}

int StreamFake_CloseCallCount(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->CloseCallCount;
}

void StreamFake_SetVersion(struct SolidSyslogStream* stream, uint32_t value)
{
    ((struct StreamFake*) stream)->Version = value;
}

void StreamFake_RefuseHandshake(struct SolidSyslogStream* stream, bool refused)
{
    ((struct StreamFake*) stream)->HandshakeRefused = refused;
}

size_t StreamFake_FrameCount(struct SolidSyslogStream* stream)
{
    return ((struct StreamFake*) stream)->FrameCount;
}

const char* StreamFake_Frame(struct SolidSyslogStream* stream, size_t index)
{
    struct StreamFake* fake = (struct StreamFake*) stream;
    return &fake->Frames[fake->FrameOffsets[index]];
}
