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
