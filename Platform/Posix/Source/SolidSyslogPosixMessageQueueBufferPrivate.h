#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERPRIVATE_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERPRIVATE_H

#include <mqueue.h>
#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogFormatter.h"

enum
{
    POSIX_MESSAGE_QUEUE_BUFFER_MAX_NAME_SIZE = 64
};

struct SolidSyslogPosixMessageQueueBuffer
{
    struct SolidSyslogBuffer Base;
    mqd_t Mq;
    SolidSyslogFormatterStorage
        NameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(POSIX_MESSAGE_QUEUE_BUFFER_MAX_NAME_SIZE)];
    size_t MaxMessageSize;
};

bool PosixMessageQueueBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    size_t maxMessageSize,
    long maxMessages,
    size_t slotIndex
);
void PosixMessageQueueBuffer_Cleanup(struct SolidSyslogBuffer* base);

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERPRIVATE_H */
