/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFileErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogFileErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_FILE_ERROR_MAX] = {
        [SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_FILE_ERROR_NULL_FILESYSTEM] = "NULL_FILESYSTEM",
        [SOLIDSYSLOG_FILE_ERROR_BUFFER_TOO_SMALL] = "BUFFER_TOO_SMALL",
        [SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED] = "OPEN_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED] = "WRITE_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_SYNC_FAILED] = "SYNC_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_COMMIT_FAILED] = "COMMIT_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_READ_FAILED] = "READ_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED] = "SEEK_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED] = "SIZE_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED] = "TRUNCATE_FAILED",
        [SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED] = "CLOSE_FAILED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_FILE_ERROR_MAX, detail);
}
