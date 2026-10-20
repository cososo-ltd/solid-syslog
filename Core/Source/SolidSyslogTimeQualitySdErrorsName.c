/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTimeQualitySdErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
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
    return SolidSyslogErrorName_Lookup(
        TimeQualitySdErrorsName_Names,
        (int32_t) SOLIDSYSLOG_TIME_QUALITY_SD_ERROR_MAX,
        detail
    );
}
