/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTlsCredentialsErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const TlsCredentialsErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "NULL_CONFIG",
    "NULL_RNG",
    "NULL_PEER_FINGERPRINT",
    "ALREADY_IN_USE",
    "PEM_NOT_TERMINATED",
    "TRUST_ANCHORS_NOT_LOADED",
    "TRUST_ANCHORS_NOT_PARSED",
    "CLIENT_CREDENTIAL_INCOMPLETE",
    "CLIENT_CREDENTIAL_NOT_PARSED",
    "CLIENT_CREDENTIAL_MISMATCHED",
    "CLIENT_CREDENTIAL_NOT_INSTALLED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(TlsCredentialsErrorsName_Names) / sizeof(TlsCredentialsErrorsName_Names[0])) ==
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_MAX,
    every_tls_credentials_error_code_has_a_name
);

const char* SolidSyslogTlsCredentialsErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        TlsCredentialsErrorsName_Names,
        (int32_t) SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_MAX,
        detail
    );
}
