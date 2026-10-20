/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAtomicCounterErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const AtomicCounterErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(AtomicCounterErrorsName_Names) / sizeof(AtomicCounterErrorsName_Names[0])) ==
        SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_MAX,
    every_atomic_counter_error_code_has_a_name
);

const char* SolidSyslogAtomicCounterErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_MAX))
    {
        name = AtomicCounterErrorsName_Names[detail];
    }
    return name;
}
