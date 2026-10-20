/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCircularBufferErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const CircularBufferErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "RECORD_TOO_LARGE",
    "RECORD_REFUSED",
    "RECORD_EXCEEDS_MAX_MESSAGE_SIZE",
    "RECORDS_EVICTED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(CircularBufferErrorsName_Names) / sizeof(CircularBufferErrorsName_Names[0])) ==
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX,
    every_circular_buffer_error_code_has_a_name
);

const char* SolidSyslogCircularBufferErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX))
    {
        name = CircularBufferErrorsName_Names[detail];
    }
    return name;
}
