/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPassthroughBufferErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogPassthroughBufferErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_MAX] = {
        [SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_NULL_SENDER] = "NULL_SENDER",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_MAX, detail);
}
