#include "SenderFake.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogTunables.h"
#include "MinSize.h"

struct SenderFake
{
    struct SolidSyslogSender Base;
    int SendCount;
    int DisconnectCount;
    char LastBuffer[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t LastSize;
    bool FailNextSend;
};

static bool Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    struct SenderFake* fake = (struct SenderFake*) self;
    size_t copySize = MinSize(size, sizeof(fake->LastBuffer) - 1);
    memcpy(fake->LastBuffer, buffer, copySize);
    fake->LastBuffer[copySize] = '\0';
    fake->LastSize = size;
    fake->SendCount++;

    if (fake->FailNextSend)
    {
        fake->FailNextSend = false;
        return false;
    }
    return true;
}

static void Disconnect(struct SolidSyslogSender* self)
{
    struct SenderFake* fake = (struct SenderFake*) self;
    fake->DisconnectCount++;
}

struct SolidSyslogSender* SenderFake_Create(void)
{
    struct SenderFake* fake = (struct SenderFake*) calloc(1, sizeof(struct SenderFake));
    fake->Base.Send = Send;
    fake->Base.Disconnect = Disconnect;
    return &fake->Base;
}

void SenderFake_Destroy(struct SolidSyslogSender* sender)
{
    free(sender);
}

void SenderFake_Reset(struct SolidSyslogSender* sender)
{
    struct SenderFake* fake = (struct SenderFake*) sender;
    fake->SendCount = 0;
    fake->DisconnectCount = 0;
    fake->LastBuffer[0] = '\0';
    fake->LastSize = 0;
    fake->FailNextSend = false;
}

int SenderFake_SendCallCount(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->SendCount;
}

int SenderFake_DisconnectCallCount(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->DisconnectCount;
}

const char* SenderFake_LastBufferAsString(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->LastBuffer;
}

size_t SenderFake_LastSize(struct SolidSyslogSender* sender)
{
    return ((struct SenderFake*) sender)->LastSize;
}

void SenderFake_FailNextSend(struct SolidSyslogSender* sender)
{
    ((struct SenderFake*) sender)->FailNextSend = true;
}
