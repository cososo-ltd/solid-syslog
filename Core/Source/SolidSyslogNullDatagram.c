#include "SolidSyslogNullDatagram.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogUdpPayload.h"

struct SolidSyslogAddress;

static bool NullDatagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult NullDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t NullDatagram_MaxPayload(struct SolidSyslogDatagram* base);
static void NullDatagram_Close(struct SolidSyslogDatagram* base);

struct SolidSyslogDatagram* SolidSyslogNullDatagram_Get(void)
{
    static struct SolidSyslogDatagram instance = {
        .Open = NullDatagram_Open,
        .SendTo = NullDatagram_SendTo,
        .MaxPayload = NullDatagram_MaxPayload,
        .Close = NullDatagram_Close,
    };
    return &instance;
}

static bool NullDatagram_Open(struct SolidSyslogDatagram* base)
{
    (void) base;
    return true;
}

/* SendTo returns SENT so the Service algorithm treats the message as
 * delivered and drops it from the Store, rather than retaining an
 * undeliverable. Mirrors NullSender_Send's drop-on-the-floor contract. */
static enum SolidSyslogDatagramSendResult NullDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    (void) base;
    (void) buffer;
    (void) size;
    (void) addr;
    return SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
}

static size_t NullDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    (void) base;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}

static void NullDatagram_Close(struct SolidSyslogDatagram* base)
{
    (void) base;
}
