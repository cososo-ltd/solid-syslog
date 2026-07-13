/** @file
 *  Error codes and Source identity for the OpenSslHmacSha256Policy adapter. */
#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICYERRORS_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICYERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is OpenSslHmacSha256PolicyErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogOpenSslHmacSha256PolicyErrors
    {
        OPENSSLHMACSHA256POLICY_ERROR_POOL_EXHAUSTED,
        OPENSSLHMACSHA256POLICY_ERROR_UNKNOWN_DESTROY,
        OPENSSLHMACSHA256POLICY_ERROR_BAD_CONFIG,
        OPENSSLHMACSHA256POLICY_ERROR_KEY_UNAVAILABLE,
        OPENSSLHMACSHA256POLICY_ERROR_KEY_TOO_SHORT,
        OPENSSLHMACSHA256POLICY_ERROR_HMAC_FAILED,
        OPENSSLHMACSHA256POLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an OpenSslHmacSha256Policy. A handler matches
     *  by address (event->Source == &OpenSslHmacSha256PolicyErrorSource), then
     *  reads event->Detail as an enum SolidSyslogOpenSslHmacSha256PolicyErrors. */
    extern const struct SolidSyslogErrorSource OpenSslHmacSha256PolicyErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICYERRORS_H */
