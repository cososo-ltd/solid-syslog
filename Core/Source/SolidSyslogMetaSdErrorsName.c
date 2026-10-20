/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogMetaSdErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const MetaSdErrorsName_Names[] = {
    [SOLIDSYSLOG_META_SD_ERROR_NULL_CONFIG] = "NULL_CONFIG",
    [SOLIDSYSLOG_META_SD_ERROR_NULL_COUNTER] = "NULL_COUNTER",
    [SOLIDSYSLOG_META_SD_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_META_SD_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(MetaSdErrorsName_Names) / sizeof(MetaSdErrorsName_Names[0])) == SOLIDSYSLOG_META_SD_ERROR_MAX,
    every_meta_sd_error_code_has_a_name
);

const char* SolidSyslogMetaSdErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(MetaSdErrorsName_Names, (int32_t) SOLIDSYSLOG_META_SD_ERROR_MAX, detail);
}
