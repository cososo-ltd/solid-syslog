/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogDatagramErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogDatagramErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_DATAGRAM_ERROR_MAX] = {
        [SOLIDSYSLOG_DATAGRAM_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED] = "NEXT_HOP_UNRESOLVED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_DATAGRAM_ERROR_MAX, detail);
}
