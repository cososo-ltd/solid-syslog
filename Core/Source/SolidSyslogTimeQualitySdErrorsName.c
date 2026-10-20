/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTimeQualitySdErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const TimeQualitySdErrorsName_Names[] = {
    "NULL_CALLBACK",
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(TimeQualitySdErrorsName_Names) / sizeof(TimeQualitySdErrorsName_Names[0])) ==
        SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_MAX,
    every_time_quality_sd_error_code_has_a_name
);

const char* SolidSyslogTimeQualitySdErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_MAX))
    {
        name = TimeQualitySdErrorsName_Names[detail];
    }
    return name;
}
