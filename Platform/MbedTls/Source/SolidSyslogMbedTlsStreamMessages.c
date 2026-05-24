#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsStreamErrors.h"

static const char* MbedTlsStreamError_AsString(uint8_t code)
{
    static const char* const messages[MBEDTLSSTREAM_ERROR_MAX] = {
        [MBEDTLSSTREAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogMbedTlsStream_Create pool exhausted; returning fallback stream",
        [MBEDTLSSTREAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogMbedTlsStream_Destroy called with a handle not issued by this pool",
        [MBEDTLSSTREAM_ERROR_DEFAULTS_NOT_APPLIED] =
            "TLS client could not be configured with its default protocol settings; connection not established",
        [MBEDTLSSTREAM_ERROR_SESSION_INIT_FAILED] =
            "TLS session could not be initialised against the configured context; connection not established",
        [MBEDTLSSTREAM_ERROR_SERVER_NAME_NOT_SET] =
            "TLS server name (SNI / cert match) could not be configured; connection not established",
        [MBEDTLSSTREAM_ERROR_HANDSHAKE_REJECTED] =
            "TLS handshake rejected by peer or local verification; connection not established",
        [MBEDTLSSTREAM_ERROR_HANDSHAKE_TIMEOUT] =
            "TLS handshake did not complete within the bounded retry budget; connection not established",
    };
    const char* result = "unknown";
    if (code < (uint8_t) MBEDTLSSTREAM_ERROR_MAX)
    {
        enum SolidSyslogMbedTlsStreamErrors typed = (enum SolidSyslogMbedTlsStreamErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource MbedTlsStreamErrorSource = {"MbedTlsStream", MbedTlsStreamError_AsString};
