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
    struct SolidSyslogDatagram Base;
    int OpenCallCount;
    int SendCallCount;
    int MaxPayloadCallCount;
    int CloseCallCount;
    enum SolidSyslogDatagramSendResult SendResults[DATAGRAMFAKE_MAX_SEND_CALLS];
    size_t MaxPayload;
    const void* SendBuffers[DATAGRAMFAKE_MAX_SEND_CALLS];
    size_t SendSizes[DATAGRAMFAKE_MAX_SEND_CALLS];
};

static bool Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t MaxPayload(struct SolidSyslogDatagram* self);
static void Close(struct SolidSyslogDatagram* self);

struct SolidSyslogDatagram* DatagramFake_Create(void)
{
    struct DatagramFake* fake = (struct DatagramFake*) calloc(1, sizeof(struct DatagramFake));
    fake->Base.Open = Open;
    fake->Base.SendTo = SendTo;
    fake->Base.MaxPayload = MaxPayload;
    fake->Base.Close = Close;
    for (int i = 0; i < DATAGRAMFAKE_MAX_SEND_CALLS; i++)
    {
        fake->SendResults[i] = SolidSyslogDatagramSendResult_Sent;
    }
    return &fake->Base;
}

void DatagramFake_Destroy(struct SolidSyslogDatagram* datagram)
{
    free(datagram);
}

void DatagramFake_SetSendResult(
    struct SolidSyslogDatagram* datagram,
    int callIndex,
    enum SolidSyslogDatagramSendResult result
)
{
    if ((callIndex >= 0) && (callIndex < DATAGRAMFAKE_MAX_SEND_CALLS))
    {
        ((struct DatagramFake*) datagram)->SendResults[callIndex] = result;
    }
}

void DatagramFake_SetMaxPayload(struct SolidSyslogDatagram* datagram, size_t maxPayload)
{
    ((struct DatagramFake*) datagram)->MaxPayload = maxPayload;
}

int DatagramFake_OpenCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->OpenCallCount;
}

int DatagramFake_SendCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->SendCallCount;
}

int DatagramFake_MaxPayloadCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->MaxPayloadCallCount;
}

int DatagramFake_CloseCallCount(struct SolidSyslogDatagram* datagram)
{
    return ((struct DatagramFake*) datagram)->CloseCallCount;
}

const void* DatagramFake_SendBuffer(struct SolidSyslogDatagram* datagram, int callIndex)
{
    if ((callIndex < 0) || (callIndex >= DATAGRAMFAKE_MAX_SEND_CALLS))
    {
        return NULL;
    }
    return ((struct DatagramFake*) datagram)->SendBuffers[callIndex];
}

size_t DatagramFake_SendSize(struct SolidSyslogDatagram* datagram, int callIndex)
{
    if ((callIndex < 0) || (callIndex >= DATAGRAMFAKE_MAX_SEND_CALLS))
    {
        return 0;
    }
    return ((struct DatagramFake*) datagram)->SendSizes[callIndex];
}

static bool Open(struct SolidSyslogDatagram* self)
{
    struct DatagramFake* fake = (struct DatagramFake*) self;
    fake->OpenCallCount++;
    return true;
}

static enum SolidSyslogDatagramSendResult SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct DatagramFake* fake = (struct DatagramFake*) self;
    int idx = fake->SendCallCount;
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagramSendResult_Failed;
    (void) addr;
    if (idx < DATAGRAMFAKE_MAX_SEND_CALLS)
    {
        fake->SendBuffers[idx] = buffer;
        fake->SendSizes[idx] = size;
        result = fake->SendResults[idx];
    }
    fake->SendCallCount++;
    return result;
}

static size_t MaxPayload(struct SolidSyslogDatagram* self)
{
    struct DatagramFake* fake = (struct DatagramFake*) self;
    fake->MaxPayloadCallCount++;
    return fake->MaxPayload;
}

static void Close(struct SolidSyslogDatagram* self)
{
    ((struct DatagramFake*) self)->CloseCallCount++;
}
