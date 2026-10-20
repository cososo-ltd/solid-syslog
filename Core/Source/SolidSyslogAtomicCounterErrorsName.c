/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAtomicCounterErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogAtomicCounterErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_MAX] = {
        [SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_MAX, detail);
}
