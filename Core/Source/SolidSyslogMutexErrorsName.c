/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogMutexErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const MutexErrorsName_Names[] = {
    [SOLIDSYSLOG_MUTEX_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_MUTEX_ERROR_CREATE_FAILED] = "CREATE_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(MutexErrorsName_Names) / sizeof(MutexErrorsName_Names[0])) == SOLIDSYSLOG_MUTEX_ERROR_MAX,
    every_mutex_error_code_has_a_name
);

const char* SolidSyslogMutexErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(MutexErrorsName_Names, (int32_t) SOLIDSYSLOG_MUTEX_ERROR_MAX, detail);
}
