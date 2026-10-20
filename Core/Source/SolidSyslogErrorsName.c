/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_ERROR_MAX] = {
        [SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG] = "CREATE_NULL_CONFIG",
        [SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER] = "CREATE_NULL_BUFFER",
        [SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER] = "CREATE_NULL_SENDER",
        [SOLIDSYSLOG_ERROR_CREATE_NULL_STORE] = "CREATE_NULL_STORE",
        [SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD] = "CREATE_INCONSISTENT_SD",
        [SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE] = "LOG_NULL_MESSAGE",
        [SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE] = "LOG_NULL_HANDLE",
        [SOLIDSYSLOG_ERROR_LOG_INCONSISTENT_SD] = "LOG_INCONSISTENT_SD",
        [SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE] = "SERVICE_NULL_HANDLE",
        [SOLIDSYSLOG_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_ERROR_HELD_RECORD_DROPPED] = "HELD_RECORD_DROPPED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_ERROR_MAX, detail);
}
