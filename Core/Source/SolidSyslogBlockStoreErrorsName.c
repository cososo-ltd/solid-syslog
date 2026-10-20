/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogBlockStoreErrors.h"

#include <stddef.h>
#include <stdint.h>

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
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX))
    {
        name = BlockStoreErrorsName_Names[detail];
    }
    return name;
}
