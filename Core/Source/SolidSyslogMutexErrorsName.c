/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogMutexErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const MutexErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "CREATE_FAILED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(MutexErrorsName_Names) / sizeof(MutexErrorsName_Names[0])) == SOLIDSYSLOG_MUTEX_ERROR_MAX,
    every_mutex_error_code_has_a_name
);

const char* SolidSyslogMutexErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_MUTEX_ERROR_MAX))
    {
        name = MutexErrorsName_Names[detail];
    }
    return name;
}
