/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAesGcmPolicyErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const AesGcmPolicyErrorsName_Names[] = {
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_BAD_CONFIG] = "BAD_CONFIG",
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE] = "KEY_UNAVAILABLE",
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_NONCE_FAILED] = "NONCE_FAILED",
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED] = "ENCRYPT_FAILED",
    [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_DECRYPT_FAILED] = "DECRYPT_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(AesGcmPolicyErrorsName_Names) / sizeof(AesGcmPolicyErrorsName_Names[0])) ==
        (size_t) SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX,
    every_aes_gcm_policy_error_code_has_a_name
);

const char* SolidSyslogAesGcmPolicyErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        AesGcmPolicyErrorsName_Names,
        (int32_t) SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX,
        detail
    );
}
