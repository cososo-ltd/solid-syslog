#include "ScratchFake.h"

#include <stddef.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogTunables.h"

static struct SolidSyslogScratch instance;
static SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
static size_t messageBytesOffered;
static int callCount;

static void WithScratch(struct SolidSyslogScratch* self, SolidSyslogScratchUser use, void* context);

struct SolidSyslogScratch* ScratchFake_Create(void)
{
    instance.WithScratch = WithScratch;
    messageBytesOffered = SOLIDSYSLOG_MAX_MESSAGE_SIZE;
    callCount = 0;
    return &instance;
}

void ScratchFake_Destroy(void)
{
    instance.WithScratch = NULL;
    callCount = 0;
}

void ScratchFake_SetMessageBytes(size_t messageBytes)
{
    messageBytesOffered = messageBytes;
}

int ScratchFake_CallCount(void)
{
    return callCount;
}

static void WithScratch(struct SolidSyslogScratch* self, SolidSyslogScratchUser use, void* context)
{
    (void) self;
    callCount++;
    use(storage, messageBytesOffered, context);
}
