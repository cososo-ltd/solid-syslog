#include "SolidSyslogNullSender.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogSenderDefinition.h"

static bool NullSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size);
static void NullSender_Disconnect(struct SolidSyslogSender* base);

struct SolidSyslogSender* SolidSyslogNullSender_Get(void)
{
    static struct SolidSyslogSender instance = {.Send = NullSender_Send, .Disconnect = NullSender_Disconnect};
    return &instance;
}

/* Send returns true ("delivered") so the Service algorithm drops the
 * message rather than retaining it in the Store. A misconfigured Sender
 * paired with a real Store would otherwise accumulate undeliverables
 * forever; returning true at the null-object boundary contains that. */
static bool NullSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size)
{
    (void) base;
    (void) buffer;
    (void) size;
    return true;
}

static void NullSender_Disconnect(struct SolidSyslogSender* base)
{
    (void) base;
}
