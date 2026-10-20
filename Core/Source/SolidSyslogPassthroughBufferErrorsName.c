/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPassthroughBufferErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const PassthroughBufferErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "NULL_SENDER",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(PassthroughBufferErrorsName_Names) / sizeof(PassthroughBufferErrorsName_Names[0])) ==
        SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_MAX,
    every_passthrough_buffer_error_code_has_a_name
);

const char* SolidSyslogPassthroughBufferErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_MAX))
    {
        name = PassthroughBufferErrorsName_Names[detail];
    }
    return name;
}
