/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCircularBufferErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const CircularBufferErrorsName_Names[] = {
    [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_TOO_LARGE] = "RECORD_TOO_LARGE",
    [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_REFUSED] = "RECORD_REFUSED",
    [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_EXCEEDS_MAX_MESSAGE_SIZE] = "RECORD_EXCEEDS_MAX_MESSAGE_SIZE",
    [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORDS_EVICTED] = "RECORDS_EVICTED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(CircularBufferErrorsName_Names) / sizeof(CircularBufferErrorsName_Names[0])) ==
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX,
    every_circular_buffer_error_code_has_a_name
);

const char* SolidSyslogCircularBufferErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        CircularBufferErrorsName_Names,
        (int32_t) SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX,
        detail
    );
}
