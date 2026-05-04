#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSender.h"

bool SolidSyslogSender_Send(struct SolidSyslogSender* sender, const void* buffer, size_t size)
{
    return sender->Send(sender, buffer, size);
}

void SolidSyslogSender_Disconnect(struct SolidSyslogSender* sender)
{
    sender->Disconnect(sender);
}
