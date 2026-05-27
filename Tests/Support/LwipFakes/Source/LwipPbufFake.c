#include "LwipPbufFake.h"

#include <stddef.h>

#include "lwip/arch.h"
#include "lwip/pbuf.h"

static unsigned pbufAllocCallCount = 0;
static struct pbuf fakePbuf;
static struct pbuf* lastAllocReturned = NULL;
static pbuf_layer lastAllocLayer = PBUF_TRANSPORT;
static u16_t lastAllocLength = 0;
static pbuf_type lastAllocType = PBUF_REF;
static bool pbufAllocFails = false;

static unsigned pbufFreeCallCount = 0;

static int outstandingPbufCount = 0;

void LwipPbufFake_Reset(void)
{
    pbufAllocCallCount = 0;
    lastAllocReturned = NULL;
    lastAllocLayer = PBUF_TRANSPORT;
    lastAllocLength = 0;
    lastAllocType = PBUF_REF;
    pbufAllocFails = false;
    pbufFreeCallCount = 0;
    outstandingPbufCount = 0;
}

void LwipPbufFake_SetPbufAllocFails(bool fails)
{
    pbufAllocFails = fails;
}

unsigned LwipPbufFake_PbufAllocCallCount(void)
{
    return pbufAllocCallCount;
}

pbuf_layer LwipPbufFake_LastAllocLayer(void)
{
    return lastAllocLayer;
}

uint16_t LwipPbufFake_LastAllocLength(void)
{
    return lastAllocLength;
}

pbuf_type LwipPbufFake_LastAllocType(void)
{
    return lastAllocType;
}

struct pbuf* LwipPbufFake_LastAllocReturned(void)
{
    return lastAllocReturned;
}

unsigned LwipPbufFake_PbufFreeCallCount(void)
{
    return pbufFreeCallCount;
}

int LwipPbufFake_OutstandingPbufCount(void)
{
    return outstandingPbufCount;
}

void LwipPbufFake_NoteIncomingPbuf(void)
{
    ++outstandingPbufCount;
}

struct pbuf* pbuf_alloc(pbuf_layer layer, u16_t length, pbuf_type type)
{
    ++pbufAllocCallCount;
    lastAllocLayer = layer;
    lastAllocLength = length;
    lastAllocType = type;
    if (pbufAllocFails)
    {
        lastAllocReturned = NULL;
    }
    else
    {
        fakePbuf.payload = NULL;
        fakePbuf.len = length;
        fakePbuf.tot_len = length;
        lastAllocReturned = &fakePbuf;
        ++outstandingPbufCount;
    }
    return lastAllocReturned;
}

u8_t pbuf_free(struct pbuf* p)
{
    (void) p;
    ++pbufFreeCallCount;
    --outstandingPbufCount;
    return 1;
}
