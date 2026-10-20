/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAesGcmPolicyErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const AesGcmPolicyErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "BAD_CONFIG",
    "KEY_UNAVAILABLE",
    "NONCE_FAILED",
    "ENCRYPT_FAILED",
    "DECRYPT_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(AesGcmPolicyErrorsName_Names) / sizeof(AesGcmPolicyErrorsName_Names[0])) ==
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX,
    every_aes_gcm_policy_error_code_has_a_name
);

const char* SolidSyslogAesGcmPolicyErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX))
    {
        name = AesGcmPolicyErrorsName_Names[detail];
    }
    return name;
}
