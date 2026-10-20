/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogMetaSdErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const MetaSdErrorsName_Names[] = {
    "NULL_CONFIG",
    "NULL_COUNTER",
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(MetaSdErrorsName_Names) / sizeof(MetaSdErrorsName_Names[0])) == SOLIDSYSLOG_META_SD_ERROR_MAX,
    every_meta_sd_error_code_has_a_name
);

const char* SolidSyslogMetaSdErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_META_SD_ERROR_MAX))
    {
        name = MetaSdErrorsName_Names[detail];
    }
    return name;
}
