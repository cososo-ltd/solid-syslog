#include "SolidSyslogNullStream.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogAddress;

static bool NullStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool NullStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize NullStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void NullStream_Close(struct SolidSyslogStream* base);

struct SolidSyslogStream* SolidSyslogNullStream_Get(void)
{
    static struct SolidSyslogStream instance = {
        .Open = NullStream_Open,
        .Send = NullStream_Send,
        .Read = NullStream_Read,
        .Close = NullStream_Close,
    };
    return &instance;
}

static bool NullStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    (void) base;
    (void) addr;
    return true;
}

/* Send returns true so the StreamSender treats the message as delivered
 * and the Store does not retain undeliverables. Mirrors NullSender_Send
 * and NullDatagram_SendTo. */
static bool NullStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    (void) base;
    (void) buffer;
    (void) size;
    return true;
}

/* Read returns 0 ("would-block, nothing available") rather than < 0 (EOF / error)
 * so StreamSender does not flag a broken connection and tear it down. */
static SolidSyslogSsize NullStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    (void) base;
    (void) buffer;
    (void) size;
    return 0;
}

static void NullStream_Close(struct SolidSyslogStream* base)
{
    (void) base;
}
