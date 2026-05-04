#include "DatagramFake.h"

#include <stdlib.h>
#include <stdbool.h>

#include "SolidSyslogDatagramDefinition.h"

enum
{
    DATAGRAMFAKE_MAX_SEND_CALLS = 4
};

struct DatagramFake
{
    struct SolidSyslogDatagram         base;
    int                                openCallCount;
    int                                sendCallCount;
    int                                maxPayloadCallCount;
    int                                closeCallCount;
    enum SolidSyslogDatagramSendResult sendResults[DATAGRAMFAKE_MAX_SEND_CALLS];
    size_t                             maxPayload;
    const void*                        sendBuffers[DATAGRAMFAKE_MAX_SEND_CALLS];
    size_t                             sendSizes[DATAGRAMFAKE_MAX_SEND_CALLS];
};

static bool                               Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr);
static size_t                             MaxPayload(struct SolidSyslogDatagram* self);
static void                               Close(struct SolidSyslogDatagram* self);

struct SolidSyslogDatagram* DatagramFake_Create(void)
{
    struct DatagramFake* fake = (struct DatagramFake*) calloc(1, sizeof(struct DatagramFake));
    fake->base.Open           = Open;
    fake->base.SendTo         = SendTo;
    fake->base.MaxPayload     = MaxPayload;
    fake->base.Close          = Close;
    for (int i = 0; i < DATAGRAMFAKE_MAX_SEND_CALLS; i++)
    {
        fake->sendResults[i] = SOLIDSYSLOG_DATAGRAM_SENT;
    }
    return &fake->base;
}

void DatagramFake_Destroy(struct SolidSyslogDatagram* datagram)
{
    free(datagram);
}

void DatagramFake_SetSendResult(struct SolidSyslogDatagram* datagram, int callIndex, enum SolidSyslogDatagramSendResult result)
{
    if ((callIndex >= 0) && (callIndex < DATAGRAMFAKE_MAX_SEND_CALLS))
    {
        ((struct DatagramFake*) datagram)->sendResults[callIndex] = result;
    }
}

void DatagramFake_SetMaxPayload(struct SolidSyslogDatagram* datagram, size_t maxPayload)
{
    ((struct DatagramFake*) datagram)->maxPayload = maxPayload;
}

int DatagramFake_OpenCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->openCallCount;
}

int DatagramFake_SendCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->sendCallCount;
}

int DatagramFake_MaxPayloadCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->maxPayloadCallCount;
}

int DatagramFake_CloseCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->closeCallCount;
}

const void* DatagramFake_SendBuffer(struct SolidSyslogDatagram* datagram, int callIndex)
{
    if ((callIndex < 0) || (callIndex >= DATAGRAMFAKE_MAX_SEND_CALLS))
    {
        return NULL;
    }
    return ((struct DatagramFake*) datagram)->sendBuffers[callIndex];
}

size_t DatagramFake_SendSize(struct SolidSyslogDatagram* datagram, int callIndex)
{
    if ((callIndex < 0) || (callIndex >= DATAGRAMFAKE_MAX_SEND_CALLS))
    {
        return 0;
    }
    return ((struct DatagramFake*) datagram)->sendSizes[callIndex];
}

static bool Open(struct SolidSyslogDatagram* self)
{
    struct DatagramFake* fake = (struct DatagramFake*) self;
    fake->openCallCount++;
    return true;
}

static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr)
{
    struct DatagramFake*               fake   = (struct DatagramFake*) self;
    int                                idx    = fake->sendCallCount;
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_FAILED;
    (void) addr;
    if (idx < DATAGRAMFAKE_MAX_SEND_CALLS)
    {
        fake->sendBuffers[idx] = buffer;
        fake->sendSizes[idx]   = size;
        result                 = fake->sendResults[idx];
    }
    fake->sendCallCount++;
    return result;
}

static size_t MaxPayload(struct SolidSyslogDatagram* self)
{
    struct DatagramFake* fake = (struct DatagramFake*) self;
    fake->maxPayloadCallCount++;
    return fake->maxPayload;
}

static void Close(struct SolidSyslogDatagram* self)
{
    ((struct DatagramFake*) self)->closeCallCount++;
}
