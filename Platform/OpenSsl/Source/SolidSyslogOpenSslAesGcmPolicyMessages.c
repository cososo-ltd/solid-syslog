#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogOpenSslAesGcmPolicyErrors.h"
#include "SolidSyslogOpenSslAesGcmPolicyPrivate.h"
#include "SolidSyslogPrival.h"

static const char* OpenSslAesGcmPolicyError_AsString(uint8_t code)
{
    static const char* const messages[OPENSSLAESGCMPOLICY_ERROR_MAX] = {
        [OPENSSLAESGCMPOLICY_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogOpenSslAesGcmPolicy_Create pool exhausted; returning fallback policy",
        [OPENSSLAESGCMPOLICY_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogOpenSslAesGcmPolicy_Destroy called with a handle not issued by this pool",
        [OPENSSLAESGCMPOLICY_ERROR_BAD_CONFIG] =
            "SolidSyslogOpenSslAesGcmPolicy_Create given a NULL config or NULL GetKey; returning fallback policy",
        [OPENSSLAESGCMPOLICY_ERROR_KEY_UNAVAILABLE] =
            "GetKey reported the AES-256 key is unavailable or not 32 bytes; record could not be sealed or opened",
        [OPENSSLAESGCMPOLICY_ERROR_NONCE_FAILED] =
            "OpenSSL RAND_bytes failed to generate a nonce; record could not be sealed",
        [OPENSSLAESGCMPOLICY_ERROR_ENCRYPT_FAILED] =
            "OpenSSL AES-256-GCM encryption failed; record could not be sealed",
        [OPENSSLAESGCMPOLICY_ERROR_DECRYPT_FAILED] =
            "OpenSSL AES-256-GCM decryption failed; record could not be opened",
    };
    const char* result = "unknown";
    if (code < (uint8_t) OPENSSLAESGCMPOLICY_ERROR_MAX)
    {
        enum SolidSyslogOpenSslAesGcmPolicyErrors typed = (enum SolidSyslogOpenSslAesGcmPolicyErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource OpenSslAesGcmPolicyErrorSource = {
    "OpenSslAesGcmPolicy",
    OpenSslAesGcmPolicyError_AsString
};

void OpenSslAesGcmPolicy_Report(enum SolidSyslogSeverity severity, enum SolidSyslogOpenSslAesGcmPolicyErrors code)
{
    SolidSyslog_Error(severity, &OpenSslAesGcmPolicyErrorSource, (uint8_t) code);
}
