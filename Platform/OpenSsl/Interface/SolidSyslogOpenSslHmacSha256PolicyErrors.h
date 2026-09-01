/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the OpenSslHmacSha256Policy adapter. */
#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICYERRORS_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICYERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogOpenSslHmacSha256PolicyErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogOpenSslHmacSha256PolicyErrors
    {
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_BAD_CONFIG,
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_KEY_UNAVAILABLE,
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_KEY_TOO_SHORT,
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_HMAC_FAILED,
        SOLIDSYSLOG_OPENSSL_HMAC_SHA256_POLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an OpenSslHmacSha256Policy. A handler matches
     *  by address (event->Source == &SolidSyslogOpenSslHmacSha256PolicyErrorSource), then
     *  reads event->Detail as an enum SolidSyslogOpenSslHmacSha256PolicyErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogOpenSslHmacSha256PolicyErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICYERRORS_H */
