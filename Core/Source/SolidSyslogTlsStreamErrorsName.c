/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTlsStreamErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const TlsStreamErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "LIBRARY_OUT_OF_MEMORY",
    "NULL_CONFIG",
    "NULL_TRANSPORT",
    "NULL_SLEEP",
    "NULL_CREDENTIALS",
    "NULL_RNG",
    "CONTEXT_INIT_FAILED",
    "DEFAULTS_NOT_APPLIED",
    "SESSION_INIT_FAILED",
    "SERVER_NAME_NOT_SET",
    "SERVER_NAME_NOT_APPLIED",
    "NO_PEER_AUTHORISATION",
    "FINGERPRINT_MALFORMED",
    "FINGERPRINT_SHA1",
    "FINGERPRINT_DIGEST_UNAVAILABLE",
    "CIPHER_POLICY_REJECTED",
    "HANDSHAKE_REJECTED",
    "HANDSHAKE_TIMEOUT",
    "PEER_CERTIFICATE_UNTRUSTED",
    "PEER_NAME_MISMATCHED",
    "PEER_CERTIFICATE_EXPIRED",
    "PEER_CERTIFICATE_NOT_YET_VALID",
    "PEER_FINGERPRINT_MISMATCHED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(TlsStreamErrorsName_Names) / sizeof(TlsStreamErrorsName_Names[0])) == SOLIDSYSLOG_TLS_STREAM_ERROR_MAX,
    every_tls_stream_error_code_has_a_name
);

const char* SolidSyslogTlsStreamErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_TLS_STREAM_ERROR_MAX))
    {
        name = TlsStreamErrorsName_Names[detail];
    }
    return name;
}
