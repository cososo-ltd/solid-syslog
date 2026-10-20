/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogHmacSha256PolicyErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const HmacSha256PolicyErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "BAD_CONFIG",
    "KEY_UNAVAILABLE",
    "KEY_TOO_SHORT",
    "HMAC_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(HmacSha256PolicyErrorsName_Names) / sizeof(HmacSha256PolicyErrorsName_Names[0])) ==
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_MAX,
    every_hmac_sha256_policy_error_code_has_a_name
);

const char* SolidSyslogHmacSha256PolicyErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_MAX))
    {
        name = HmacSha256PolicyErrorsName_Names[detail];
    }
    return name;
}
