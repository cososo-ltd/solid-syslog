/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAtomicCounterErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
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
    return SolidSyslogErrorName_Lookup(
        AtomicCounterErrorsName_Names,
        (int32_t) SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_MAX,
        detail
    );
}
