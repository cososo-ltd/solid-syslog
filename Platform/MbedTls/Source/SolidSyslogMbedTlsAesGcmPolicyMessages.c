#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsAesGcmPolicyErrors.h"
#include "SolidSyslogMbedTlsAesGcmPolicyPrivate.h"
#include "SolidSyslogPrival.h"

static const char* MbedTlsAesGcmPolicyError_AsString(uint8_t code)
{
    static const char* const messages[MBEDTLSAESGCMPOLICY_ERROR_MAX] = {
        [MBEDTLSAESGCMPOLICY_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogMbedTlsAesGcmPolicy_Create pool exhausted; returning fallback policy",
        [MBEDTLSAESGCMPOLICY_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogMbedTlsAesGcmPolicy_Destroy called with a handle not issued by this pool",
        [MBEDTLSAESGCMPOLICY_ERROR_BAD_CONFIG] =
            "SolidSyslogMbedTlsAesGcmPolicy_Create given a NULL config, GetKey, or Rng; returning fallback policy",
        [MBEDTLSAESGCMPOLICY_ERROR_KEY_UNAVAILABLE] =
            "GetKey reported the AES-256 key is unavailable or not 32 bytes; record could not be sealed or opened",
        [MBEDTLSAESGCMPOLICY_ERROR_NONCE_FAILED] =
            "mbedTLS CTR-DRBG failed to generate a nonce; record could not be sealed",
        [MBEDTLSAESGCMPOLICY_ERROR_ENCRYPT_FAILED] =
            "mbedTLS AES-256-GCM encryption failed; record could not be sealed",
        [MBEDTLSAESGCMPOLICY_ERROR_DECRYPT_FAILED] =
            "mbedTLS AES-256-GCM decryption failed; record could not be opened",
    };
    const char* result = "unknown";
    if (code < (uint8_t) MBEDTLSAESGCMPOLICY_ERROR_MAX)
    {
        enum SolidSyslogMbedTlsAesGcmPolicyErrors typed = (enum SolidSyslogMbedTlsAesGcmPolicyErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource MbedTlsAesGcmPolicyErrorSource = {
    "MbedTlsAesGcmPolicy",
    MbedTlsAesGcmPolicyError_AsString
};

void MbedTlsAesGcmPolicy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMbedTlsAesGcmPolicyErrors code
)
{
    SolidSyslog_Error(severity, &MbedTlsAesGcmPolicyErrorSource, category, (int32_t) code);
}
