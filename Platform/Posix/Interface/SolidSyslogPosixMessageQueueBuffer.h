#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H

#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(size_t maxMessageSize, long maxMessages);
    void                      SolidSyslogPosixMessageQueueBuffer_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H */
