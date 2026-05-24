#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogTlsStreamErrors.h"

static const char* TlsStreamError_AsString(uint8_t code)
{
    static const char* const messages[TLSSTREAM_ERROR_MAX] = {
        [TLSSTREAM_ERROR_POOL_EXHAUSTED] = "SolidSyslogTlsStream_Create pool exhausted; returning fallback stream",
        [TLSSTREAM_ERROR_UNKNOWN_DESTROY] = "SolidSyslogTlsStream_Destroy called with a handle not issued by this pool",
        [TLSSTREAM_ERROR_CONTEXT_INIT_FAILED] =
            "TLS client context could not be configured; connection not established",
        [TLSSTREAM_ERROR_SESSION_INIT_FAILED] =
            "TLS session could not be initialised against the configured context; connection not established",
        [TLSSTREAM_ERROR_SERVER_NAME_NOT_SET] =
            "TLS server name (SNI / cert match) could not be configured; connection not established",
        [TLSSTREAM_ERROR_HANDSHAKE_REJECTED] =
            "TLS handshake rejected by peer or local verification; connection not established",
        [TLSSTREAM_ERROR_HANDSHAKE_TIMEOUT] =
            "TLS handshake did not complete within the bounded retry budget; connection not established",
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
