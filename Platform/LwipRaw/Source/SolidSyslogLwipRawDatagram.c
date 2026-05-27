#include "SolidSyslogLwipRawDatagramPrivate.h"

#include <stdbool.h>
#include <stddef.h>

#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogUdpPayload.h"

struct SolidSyslogAddress;

static bool LwipRawDatagram_Open(struct SolidSyslogDatagram* base);
static void LwipRawDatagram_Close(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult LwipRawDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t LwipRawDatagram_MaxPayload(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogLwipRawDatagram* LwipRawDatagram_SelfFromBase(struct SolidSyslogDatagram* base);
static inline bool LwipRawDatagram_IsOpen(const struct SolidSyslogLwipRawDatagram* self);

void LwipRawDatagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagram_SelfFromBase(base);
    self->Base.Open = LwipRawDatagram_Open;
    self->Base.Close = LwipRawDatagram_Close;
    self->Base.SendTo = LwipRawDatagram_SendTo;
    self->Base.MaxPayload = LwipRawDatagram_MaxPayload;
    self->Pcb = NULL;
}

static inline struct SolidSyslogLwipRawDatagram* LwipRawDatagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogLwipRawDatagram*) base;
}

void LwipRawDatagram_Cleanup(struct SolidSyslogDatagram* base)
{
    LwipRawDatagram_Close(base);
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullDatagram_Get();
}

static void LwipRawDatagram_Close(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagram_SelfFromBase(base);
    if (LwipRawDatagram_IsOpen(self))
    {
        udp_remove(self->Pcb);
        self->Pcb = NULL;
    }
}

static bool LwipRawDatagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagram_SelfFromBase(base);
    if (!LwipRawDatagram_IsOpen(self))
    {
        self->Pcb = udp_new();
    }
    return LwipRawDatagram_IsOpen(self);
}

static inline bool LwipRawDatagram_IsOpen(const struct SolidSyslogLwipRawDatagram* self)
{
    return self->Pcb != NULL;
}

static enum SolidSyslogDatagramSendResult LwipRawDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagram_SelfFromBase(base);
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
    if (LwipRawDatagram_IsOpen(self))
    {
        struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, (u16_t) size, PBUF_REF);
        if (p != NULL)
        {
            const struct SolidSyslogLwipRawAddress* dst = SolidSyslogLwipRawAddress_AsConst(addr);
            p->payload = (void*) buffer;
            err_t err = udp_sendto(self->Pcb, p, &dst->Ip, dst->Port);
            (void) pbuf_free(p);
            if (err == ERR_OK)
            {
                result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
            }
        }
    }
    return result;
}

static size_t LwipRawDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    (void) base;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}
