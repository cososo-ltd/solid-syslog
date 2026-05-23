#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogTlsStreamErrors.h"

static const char* TlsStreamError_AsString(uint8_t code)
{
    static const char* const messages[TLSSTREAM_ERROR_MAX] = {
        [TLSSTREAM_ERROR_POOL_EXHAUSTED] = "SolidSyslogTlsStream_Create pool exhausted; returning fallback stream",
        [TLSSTREAM_ERROR_UNKNOWN_DESTROY] = "SolidSyslogTlsStream_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) TLSSTREAM_ERROR_MAX)
    {
        enum SolidSyslogTlsStreamErrors typed = (enum SolidSyslogTlsStreamErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource TlsStreamErrorSource = {"TlsStream", TlsStreamError_AsString};
