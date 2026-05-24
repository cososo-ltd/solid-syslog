#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"

static const char* PosixMessageQueueBufferError_AsString(uint8_t code)
{
    static const char* const messages[POSIXMESSAGEQUEUEBUFFER_ERROR_MAX] = {
        [POSIXMESSAGEQUEUEBUFFER_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogPosixMessageQueueBuffer_Create pool exhausted; returning fallback buffer",
        [POSIXMESSAGEQUEUEBUFFER_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPosixMessageQueueBuffer_Destroy called with a handle not issued by this pool",
        [POSIXMESSAGEQUEUEBUFFER_ERROR_MQ_OPEN_FAILED] =
            "SolidSyslogPosixMessageQueueBuffer_Create mq_open failed; returning fallback buffer",
        [POSIXMESSAGEQUEUEBUFFER_ERROR_SEND_FAILED] =
            "SolidSyslogPosixMessageQueueBuffer_Write mq_send failed; record dropped",
        [POSIXMESSAGEQUEUEBUFFER_ERROR_RECEIVE_FAILED] =
            "SolidSyslogPosixMessageQueueBuffer_Read mq_receive failed",
    };
    const char* result = "unknown";
    if (code < (uint8_t) POSIXMESSAGEQUEUEBUFFER_ERROR_MAX)
    {
        enum SolidSyslogPosixMessageQueueBufferErrors typed = (enum SolidSyslogPosixMessageQueueBufferErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PosixMessageQueueBufferErrorSource = {
    "PosixMessageQueueBuffer",
    PosixMessageQueueBufferError_AsString
};
