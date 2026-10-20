/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogDatagramErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const DatagramErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "NEXT_HOP_UNRESOLVED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(DatagramErrorsName_Names) / sizeof(DatagramErrorsName_Names[0])) == SOLIDSYSLOG_DATAGRAM_ERROR_MAX,
    every_datagram_error_code_has_a_name
);

const char* SolidSyslogDatagramErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(DatagramErrorsName_Names, (int32_t) SOLIDSYSLOG_DATAGRAM_ERROR_MAX, detail);
}
