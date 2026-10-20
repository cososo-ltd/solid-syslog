/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCircularBufferErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogCircularBufferErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX] = {
        [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_TOO_LARGE] = "RECORD_TOO_LARGE",
        [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_REFUSED] = "RECORD_REFUSED",
        [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_EXCEEDS_MAX_MESSAGE_SIZE] = "RECORD_EXCEEDS_MAX_MESSAGE_SIZE",
        [SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORDS_EVICTED] = "RECORDS_EVICTED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX, detail);
}
