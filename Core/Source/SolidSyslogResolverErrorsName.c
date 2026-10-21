/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogResolverErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogResolverErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_RESOLVER_ERROR_MAX] = {
        [SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_RESOLVER_ERROR_RESOLVE_TIMEOUT] = "RESOLVE_TIMEOUT",
        [SOLIDSYSLOG_RESOLVER_ERROR_NULL_CONFIG] = "NULL_CONFIG",
        [SOLIDSYSLOG_RESOLVER_ERROR_NULL_SLEEP] = "NULL_SLEEP",
        [SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED] = "ADDRESS_FAMILY_UNSUPPORTED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_RESOLVER_ERROR_MAX, detail);
}
