/** @file
 *  Error codes and Source identity for the MbedTlsAesGcmPolicy. */
#ifndef SOLIDSYSLOGMBEDTLSAESGCMPOLICYERRORS_H
#define SOLIDSYSLOGMBEDTLSAESGCMPOLICYERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is MbedTlsAesGcmPolicyErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. A tag mismatch on open is the expected
     *  tamper-detected outcome and is NOT reported — DECRYPT_FAILED is only a
     *  genuine mbedTLS error. */
    enum SolidSyslogMbedTlsAesGcmPolicyErrors
    {
        MBEDTLSAESGCMPOLICY_ERROR_POOL_EXHAUSTED,
        MBEDTLSAESGCMPOLICY_ERROR_UNKNOWN_DESTROY,
        MBEDTLSAESGCMPOLICY_ERROR_BAD_CONFIG,
        MBEDTLSAESGCMPOLICY_ERROR_KEY_UNAVAILABLE,
        MBEDTLSAESGCMPOLICY_ERROR_NONCE_FAILED,
        MBEDTLSAESGCMPOLICY_ERROR_ENCRYPT_FAILED,
        MBEDTLSAESGCMPOLICY_ERROR_DECRYPT_FAILED,
        MBEDTLSAESGCMPOLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an MbedTlsAesGcmPolicy. A handler matches by
     *  address (event->Source == &MbedTlsAesGcmPolicyErrorSource), then reads
     *  event->Detail as an enum SolidSyslogMbedTlsAesGcmPolicyErrors. */
    extern const struct SolidSyslogErrorSource MbedTlsAesGcmPolicyErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSAESGCMPOLICYERRORS_H */
