/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTimeQualitySdErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogTimeQualitySdErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_MAX] = {
        [SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_NULL_CALLBACK] = "NULL_CALLBACK",
        [SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_MAX, detail);
}
