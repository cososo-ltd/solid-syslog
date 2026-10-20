/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogResolverErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const ResolverErrorsName_Names[] = {
    [SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_RESOLVER_ERROR_RESOLVE_TIMEOUT] = "RESOLVE_TIMEOUT",
    [SOLIDSYSLOG_RESOLVER_ERROR_NULL_CONFIG] = "NULL_CONFIG",
    [SOLIDSYSLOG_RESOLVER_ERROR_NULL_SLEEP] = "NULL_SLEEP",
    [SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED] = "ADDRESS_FAMILY_UNSUPPORTED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(ResolverErrorsName_Names) / sizeof(ResolverErrorsName_Names[0])) == (size_t) SOLIDSYSLOG_RESOLVER_ERROR_MAX,
    every_resolver_error_code_has_a_name
);

const char* SolidSyslogResolverErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(ResolverErrorsName_Names, (int32_t) SOLIDSYSLOG_RESOLVER_ERROR_MAX, detail);
}
