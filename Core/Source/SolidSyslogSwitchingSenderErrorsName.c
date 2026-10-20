/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSwitchingSenderErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const SwitchingSenderErrorsName_Names[] = {
    [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_CONFIG] = "NULL_CONFIG",
    [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_SENDERS] = "NULL_SENDERS",
    [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_SELECTOR] = "NULL_SELECTOR",
    [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(SwitchingSenderErrorsName_Names) / sizeof(SwitchingSenderErrorsName_Names[0])) ==
        (size_t) SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX,
    every_switching_sender_error_code_has_a_name
);

const char* SolidSyslogSwitchingSenderErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        SwitchingSenderErrorsName_Names,
        (int32_t) SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX,
        detail
    );
}
