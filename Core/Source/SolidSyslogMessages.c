#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"

static const char* SolidSyslogError_AsString(uint8_t code)
{
    static const char* const messages[SOLIDSYSLOG_ERROR_MAX] = {
        [SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG] = "SolidSyslog_Create called with NULL config",
        [SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER] = "SolidSyslog_Create config.Buffer is NULL",
        [SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER] = "SolidSyslog_Create config.Sender is NULL",
        [SOLIDSYSLOG_ERROR_CREATE_NULL_STORE] = "SolidSyslog_Create config.Store is NULL",
        [SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD] =
            "SolidSyslog_Create config.Sd is NULL but config.SdCount is non-zero",
        [SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE] = "SolidSyslog_Log called with NULL message",
        [SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE] = "SolidSyslog_Log called with NULL handle",
        [SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE] = "SolidSyslog_Service called with NULL handle",
        [SOLIDSYSLOG_ERROR_POOL_EXHAUSTED] = "SolidSyslog_Create pool exhausted; returning fallback instance",
        [SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY] = "SolidSyslog_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) SOLIDSYSLOG_ERROR_MAX)
    {
        enum SolidSyslogErrors typed = (enum SolidSyslogErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource SolidSyslogErrorSource = {"SolidSyslog", SolidSyslogError_AsString};
