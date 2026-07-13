/** @file
 *  Error codes and Source identity for the MbedTlsHmacSha256Policy. */
#ifndef SOLIDSYSLOGMBEDTLSHMACSHA256POLICYERRORS_H
#define SOLIDSYSLOGMBEDTLSHMACSHA256POLICYERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is MbedTlsHmacSha256PolicyErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. KEY_TOO_SHORT is raised when GetKey returns a
     *  key below the SHA-256 output length (32 bytes) — the policy fails closed
     *  rather than seal with a cryptographically worthless MAC. A tag mismatch on
     *  open is silent (the expected tamper verdict), so no HMAC_FAILED for that. */
    enum SolidSyslogMbedTlsHmacSha256PolicyErrors
    {
        MBEDTLSHMACSHA256POLICY_ERROR_POOL_EXHAUSTED,
        MBEDTLSHMACSHA256POLICY_ERROR_UNKNOWN_DESTROY,
        MBEDTLSHMACSHA256POLICY_ERROR_BAD_CONFIG,
        MBEDTLSHMACSHA256POLICY_ERROR_KEY_UNAVAILABLE,
        MBEDTLSHMACSHA256POLICY_ERROR_KEY_TOO_SHORT,
        MBEDTLSHMACSHA256POLICY_ERROR_HMAC_FAILED,
        MBEDTLSHMACSHA256POLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an MbedTlsHmacSha256Policy. A handler matches
     *  by address (event->Source == &MbedTlsHmacSha256PolicyErrorSource), then
     *  reads event->Detail as an enum SolidSyslogMbedTlsHmacSha256PolicyErrors. */
    extern const struct SolidSyslogErrorSource MbedTlsHmacSha256PolicyErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSHMACSHA256POLICYERRORS_H */
