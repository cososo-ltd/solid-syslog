/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogHmacSha256PolicyErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const HmacSha256PolicyErrorsName_Names[] = {
    [SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_BAD_CONFIG] = "BAD_CONFIG",
    [SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_KEY_UNAVAILABLE] = "KEY_UNAVAILABLE",
    [SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_KEY_TOO_SHORT] = "KEY_TOO_SHORT",
    [SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_HMAC_FAILED] = "HMAC_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(HmacSha256PolicyErrorsName_Names) / sizeof(HmacSha256PolicyErrorsName_Names[0])) ==
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_MAX,
    every_hmac_sha256_policy_error_code_has_a_name
);

const char* SolidSyslogHmacSha256PolicyErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        HmacSha256PolicyErrorsName_Names,
        (int32_t) SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_MAX,
        detail
    );
}
