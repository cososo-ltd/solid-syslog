/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFileErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const FileErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "NULL_FILESYSTEM",
    "BUFFER_TOO_SMALL",
    "OPEN_FAILED",
    "WRITE_FAILED",
    "SYNC_FAILED",
    "COMMIT_FAILED",
    "READ_FAILED",
    "SEEK_FAILED",
    "SIZE_FAILED",
    "TRUNCATE_FAILED",
    "CLOSE_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(FileErrorsName_Names) / sizeof(FileErrorsName_Names[0])) == SOLIDSYSLOG_FILE_ERROR_MAX,
    every_file_error_code_has_a_name
);

const char* SolidSyslogFileErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_FILE_ERROR_MAX))
    {
        name = FileErrorsName_Names[detail];
    }
    return name;
}
