#include "LwipPbufFake.h"

#include <stddef.h>
#include <string.h>

#include "LwipFakeMarshalGuard.h"
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
    LWIP_REQUIRE_MARSHAL_ACTIVE();
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
    LWIP_REQUIRE_MARSHAL_ACTIVE();
    (void) p;
    ++pbufFreeCallCount;
    --outstandingPbufCount;
    return 1;
}

/* Faithful model of lwIP's pbuf_copy_partial: copy up to `len` bytes out of
 * the pbuf chain `p`, starting `offset` bytes in, walking `p->next` across
 * links. Returns the number of bytes copied. Lets the chained-pbuf tests
 * exercise the wrapper's tot_len-keyed drain just as real lwIP would. */
u16_t pbuf_copy_partial(const struct pbuf* p, void* dataptr, u16_t len, u16_t offset)
{
    LWIP_REQUIRE_MARSHAL_ACTIVE();
    uint8_t* out = (uint8_t*) dataptr;
    u16_t skip = offset;
    u16_t copied = 0;
    const struct pbuf* link = p;
    while ((link != NULL) && (copied < len))
    {
        if (skip >= link->len)
        {
            skip = (u16_t) (skip - link->len);
        }
        else
        {
            u16_t linkAvailable = (u16_t) (link->len - skip);
            u16_t want = (u16_t) (len - copied);
            u16_t take = (want < linkAvailable) ? want : linkAvailable;
            (void) memcpy(&out[copied], &((const uint8_t*) link->payload)[skip], take);
            copied = (u16_t) (copied + take);
            skip = 0;
        }
        link = link->next;
    }
    return copied;
}
