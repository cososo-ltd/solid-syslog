/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const ErrorsName_Names[] = {
    "CREATE_NULL_CONFIG",
    "CREATE_NULL_BUFFER",
    "CREATE_NULL_SENDER",
    "CREATE_NULL_STORE",
    "CREATE_INCONSISTENT_SD",
    "LOG_NULL_MESSAGE",
    "LOG_NULL_HANDLE",
    "LOG_INCONSISTENT_SD",
    "SERVICE_NULL_HANDLE",
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "HELD_RECORD_DROPPED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(ErrorsName_Names) / sizeof(ErrorsName_Names[0])) == SOLIDSYSLOG_ERROR_MAX,
    every_error_code_has_a_name
);

const char* SolidSyslogErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_ERROR_MAX))
    {
        name = ErrorsName_Names[detail];
    }
    return name;
}
