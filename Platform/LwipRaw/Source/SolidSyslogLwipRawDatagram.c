#include "SolidSyslogLwipRawDatagramPrivate.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawDatagramErrors.h"
#include "SolidSyslogLwipRawMarshalPrivate.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogUdpPayload.h"
#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

const struct SolidSyslogErrorSource LwipRawDatagramErrorSource = {"LwipRawDatagram"};

struct SolidSyslogAddress;

/* Per-operation parameters carried across one marshal hop. Only the fields
 * the in-flight op needs are set (Open/Close set just Self); SendTo fills the
 * rest. One struct per class so the void*-context recovery has a single cast
 * site (LwipRawDatagramCallFromContext) — see D.002 in docs/misra-deviations.md. */
struct LwipRawDatagramCall
{
    struct SolidSyslogLwipRawDatagram* Self;
    const struct SolidSyslogLwipRawAddress* Dst;
    const void* Buffer;
    size_t Size;
    enum SolidSyslogDatagramSendResult Result;
};

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
static inline struct LwipRawDatagramCall* LwipRawDatagramCallFromContext(void* context);
static inline bool LwipRawDatagram_IsOpen(const struct SolidSyslogLwipRawDatagram* self);
static void LwipRawDatagram_DoOpen(void* context);
static void LwipRawDatagram_DoClose(void* context);
static void LwipRawDatagram_DoSendTo(void* context);

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

/* Recovers the per-op call struct from the void* context the marshal passes
 * back into each Do* callback. Single named helper so the void→struct cast
 * lives in one place — one suppression site per class, not one per callback
 * (the marshal-seam analogue of LwipRawTcpStream_SelfFromArg; see D.002). */
static inline struct LwipRawDatagramCall* LwipRawDatagramCallFromContext(void* context)
{
    return (struct LwipRawDatagramCall*) context;
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
        struct LwipRawDatagramCall call = {.Self = self};
        SolidSyslogLwipRaw_Marshal(LwipRawDatagram_DoClose, &call);
    }
}

static void LwipRawDatagram_DoClose(void* context)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagramCallFromContext(context)->Self;
    udp_remove(self->Pcb);
    self->Pcb = NULL;
}

static bool LwipRawDatagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagram_SelfFromBase(base);
    if (!LwipRawDatagram_IsOpen(self))
    {
        struct LwipRawDatagramCall call = {.Self = self};
        SolidSyslogLwipRaw_Marshal(LwipRawDatagram_DoOpen, &call);
    }
    return LwipRawDatagram_IsOpen(self);
}

static void LwipRawDatagram_DoOpen(void* context)
{
    struct SolidSyslogLwipRawDatagram* self = LwipRawDatagramCallFromContext(context)->Self;
    self->Pcb = udp_new();
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
        struct LwipRawDatagramCall call = {
            .Self = self,
            .Dst = SolidSyslogLwipRawAddress_AsConst(addr),
            .Buffer = buffer,
            .Size = size,
            .Result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED,
        };
        SolidSyslogLwipRaw_Marshal(LwipRawDatagram_DoSendTo, &call);
        result = call.Result;
    }
    return result;
}

/* Runs the whole send — pbuf alloc, sendto, free — in one marshalled hop so
 * a NO_SYS=0 integrator pays a single tcpip-thread context switch per Send
 * rather than three. PBUF_REF points lwIP at the caller's buffer; the buffer
 * outlives the synchronous hop, so no copy is needed. */
static void LwipRawDatagram_DoSendTo(void* context)
{
    struct LwipRawDatagramCall* call = LwipRawDatagramCallFromContext(context);
    call->Result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, (u16_t) call->Size, PBUF_REF);
    if (p != NULL)
    {
        p->payload = (void*) call->Buffer;
        err_t err = udp_sendto(call->Self->Pcb, p, &call->Dst->Ip, call->Dst->Port);
        (void) pbuf_free(p);
        if (err == ERR_OK)
        {
            call->Result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
        }
    }
}

static size_t LwipRawDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    (void) base;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}
