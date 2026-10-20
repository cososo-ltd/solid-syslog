/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogOriginSdErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
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
    return SolidSyslogErrorName_Lookup(OriginSdErrorsName_Names, (int32_t) SOLIDSYSLOG_ORIGIN_SD_ERROR_MAX, detail);
}
