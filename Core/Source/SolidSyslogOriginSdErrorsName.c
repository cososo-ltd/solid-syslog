/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogOriginSdErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const OriginSdErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(OriginSdErrorsName_Names) / sizeof(OriginSdErrorsName_Names[0])) == SOLIDSYSLOG_ORIGIN_SD_ERROR_MAX,
    every_origin_sd_error_code_has_a_name
);

const char* SolidSyslogOriginSdErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_ORIGIN_SD_ERROR_MAX))
    {
        name = OriginSdErrorsName_Names[detail];
    }
    return name;
}
