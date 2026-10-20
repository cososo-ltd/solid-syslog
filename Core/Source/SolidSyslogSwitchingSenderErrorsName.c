/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSwitchingSenderErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const SwitchingSenderErrorsName_Names[] = {
    "NULL_CONFIG",
    "NULL_SENDERS",
    "NULL_SELECTOR",
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(SwitchingSenderErrorsName_Names) / sizeof(SwitchingSenderErrorsName_Names[0])) ==
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX,
    every_switching_sender_error_code_has_a_name
);

const char* SolidSyslogSwitchingSenderErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX))
    {
        name = SwitchingSenderErrorsName_Names[detail];
    }
    return name;
}
