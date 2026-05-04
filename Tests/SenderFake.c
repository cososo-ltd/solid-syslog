#include "SenderFake.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "SolidSyslog.h"
#include "SolidSyslogSenderDefinition.h"
#include "TestUtils.h"

struct SenderFake
{
    struct SolidSyslogSender base;
    int                      sendCount;
    int                      disconnectCount;
    char                     lastBuffer[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t                   lastSize;
    bool                     failNextSend;
};

static bool Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    struct SenderFake* fake     = (struct SenderFake*) self;
    size_t             copySize = MinSize(size, sizeof(fake->lastBuffer) - 1);
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize; memcpy_s is not portable
    memcpy(fake->lastBuffer, buffer, copySize);
    fake->lastBuffer[copySize] = '\0';
    fake->lastSize             = size;
    fake->sendCount++;

    if (fake->failNextSend)
    {
        fake->failNextSend = false;
        return false;
    }
    return true;
}

static void Disconnect(struct SolidSyslogSender* self)
{
    struct SenderFake* fake = (struct SenderFake*) self;
    fake->disconnectCount++;
}

struct SolidSyslogSender* SenderFake_Create(void)
{
    struct SenderFake* fake = (struct SenderFake*) calloc(1, sizeof(struct SenderFake));
    fake->base.Send         = Send;
    fake->base.Disconnect   = Disconnect;
    return &fake->base;
}

void SenderFake_Destroy(struct SolidSyslogSender* sender)
{
    free(sender);
}

void SenderFake_Reset(struct SolidSyslogSender* sender)
{
    struct SenderFake* fake = (struct SenderFake*) sender;
    fake->sendCount         = 0;
    fake->disconnectCount   = 0;
    fake->lastBuffer[0]     = '\0';
    fake->lastSize          = 0;
    fake->failNextSend      = false;
}

int SenderFake_SendCount(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->sendCount;
}

int SenderFake_DisconnectCount(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->disconnectCount;
}

const char* SenderFake_LastBufferAsString(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->lastBuffer;
}

size_t SenderFake_LastSize(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->lastSize;
}

void SenderFake_FailNextSend(struct SolidSyslogSender* sender)
{
    ((struct SenderFake*) sender)->failNextSend = true;
}
