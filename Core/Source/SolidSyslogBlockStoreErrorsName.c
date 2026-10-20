/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogBlockStoreErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const BlockStoreErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "BLOCK_TOO_SMALL",
    "SEAL_FAILED",
    "OPEN_FAILED",
    "APPEND_FAILED",
    "ROTATE_FAILED",
    "DISPOSE_FAILED",
    "MARK_SENT_FAILED",
    "READ_FAILED",
    "SIZE_FAILED",
    "DISCARD_FAILED",
    "RECORD_REJECTED",
    "RECORD_CORRUPT",
    "RECORD_REFUSED",
    "BLOCK_EVICTED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(BlockStoreErrorsName_Names) / sizeof(BlockStoreErrorsName_Names[0])) == SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX,
    every_block_store_error_code_has_a_name
);

const char* SolidSyslogBlockStoreErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(BlockStoreErrorsName_Names, (int32_t) SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX, detail);
}
