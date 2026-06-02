#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogOpenSslHmacSha256PolicyErrors.h"
#include "SolidSyslogOpenSslHmacSha256PolicyPrivate.h"
#include "SolidSyslogPrival.h"

static const char* OpenSslHmacSha256PolicyError_AsString(uint8_t code)
{
    static const char* const messages[OPENSSLHMACSHA256POLICY_ERROR_MAX] = {
        [OPENSSLHMACSHA256POLICY_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogOpenSslHmacSha256Policy_Create pool exhausted; returning fallback policy",
        [OPENSSLHMACSHA256POLICY_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogOpenSslHmacSha256Policy_Destroy called with a handle not issued by this pool",
        [OPENSSLHMACSHA256POLICY_ERROR_BAD_CONFIG] =
            "SolidSyslogOpenSslHmacSha256Policy_Create given a NULL config or NULL GetKey; returning fallback policy",
        [OPENSSLHMACSHA256POLICY_ERROR_KEY_UNAVAILABLE] =
            "GetKey reported the HMAC key is unavailable; record integrity could not be sealed or verified",
        [OPENSSLHMACSHA256POLICY_ERROR_HMAC_FAILED] =
            "OpenSSL HMAC-SHA256 computation failed; record integrity could not be sealed or verified",
    };
    const char* result = "unknown";
    if (code < (uint8_t) OPENSSLHMACSHA256POLICY_ERROR_MAX)
    {
        enum SolidSyslogOpenSslHmacSha256PolicyErrors typed = (enum SolidSyslogOpenSslHmacSha256PolicyErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource OpenSslHmacSha256PolicyErrorSource = {
    "OpenSslHmacSha256Policy",
    OpenSslHmacSha256PolicyError_AsString
};

void OpenSslHmacSha256Policy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOpenSslHmacSha256PolicyErrors code
)
{
    SolidSyslog_Error(severity, &OpenSslHmacSha256PolicyErrorSource, category, (int32_t) code);
}
