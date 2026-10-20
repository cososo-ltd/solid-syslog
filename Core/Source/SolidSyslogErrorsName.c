/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const ErrorsName_Names[] = {
    [SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG] = "CREATE_NULL_CONFIG",
    [SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER] = "CREATE_NULL_BUFFER",
    [SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER] = "CREATE_NULL_SENDER",
    [SOLIDSYSLOG_ERROR_CREATE_NULL_STORE] = "CREATE_NULL_STORE",
    [SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD] = "CREATE_INCONSISTENT_SD",
    [SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE] = "LOG_NULL_MESSAGE",
    [SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE] = "LOG_NULL_HANDLE",
    [SOLIDSYSLOG_ERROR_LOG_INCONSISTENT_SD] = "LOG_INCONSISTENT_SD",
    [SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE] = "SERVICE_NULL_HANDLE",
    [SOLIDSYSLOG_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_ERROR_HELD_RECORD_DROPPED] = "HELD_RECORD_DROPPED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(ErrorsName_Names) / sizeof(ErrorsName_Names[0])) == SOLIDSYSLOG_ERROR_MAX,
    every_error_code_has_a_name
);

const char* SolidSyslogErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(ErrorsName_Names, (int32_t) SOLIDSYSLOG_ERROR_MAX, detail);
}
