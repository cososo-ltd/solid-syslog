/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogResolverErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const ResolverErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "RESOLVE_TIMEOUT",
    "NULL_CONFIG",
    "NULL_SLEEP",
    "ADDRESS_FAMILY_UNSUPPORTED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(ResolverErrorsName_Names) / sizeof(ResolverErrorsName_Names[0])) == SOLIDSYSLOG_RESOLVER_ERROR_MAX,
    every_resolver_error_code_has_a_name
);

const char* SolidSyslogResolverErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(ResolverErrorsName_Names, (int32_t) SOLIDSYSLOG_RESOLVER_ERROR_MAX, detail);
}
