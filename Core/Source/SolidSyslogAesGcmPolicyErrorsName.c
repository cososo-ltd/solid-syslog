/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAesGcmPolicyErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogAesGcmPolicyErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX] = {
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_BAD_CONFIG] = "BAD_CONFIG",
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE] = "KEY_UNAVAILABLE",
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_NONCE_FAILED] = "NONCE_FAILED",
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED] = "ENCRYPT_FAILED",
        [SOLIDSYSLOG_AES_GCM_POLICY_ERROR_DECRYPT_FAILED] = "DECRYPT_FAILED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_AES_GCM_POLICY_ERROR_MAX, detail);
}
