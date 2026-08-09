/** @file
 *  Error codes and Source identity for the MbedTlsAesGcmPolicy. */
#ifndef SOLIDSYSLOGMBEDTLSAESGCMPOLICYERRORS_H
#define SOLIDSYSLOGMBEDTLSAESGCMPOLICYERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is MbedTlsAesGcmPolicyErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. A tag mismatch on open is the expected
     *  tamper-detected outcome and is NOT reported — DECRYPT_FAILED is only a
     *  genuine mbedTLS error. */
    enum SolidSyslogMbedTlsAesGcmPolicyErrors
    {
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_BAD_CONFIG,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_NONCE_FAILED,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_DECRYPT_FAILED,
        SOLIDSYSLOG_MBEDTLS_AES_GCM_POLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an MbedTlsAesGcmPolicy. A handler matches by
     *  address (event->Source == &MbedTlsAesGcmPolicyErrorSource), then reads
     *  event->Detail as an enum SolidSyslogMbedTlsAesGcmPolicyErrors. */
    extern const struct SolidSyslogErrorSource MbedTlsAesGcmPolicyErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSAESGCMPOLICYERRORS_H */
