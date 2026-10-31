/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSwitchingSenderErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogSwitchingSenderErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX] = {
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_CONFIG] = "NULL_CONFIG",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_SENDERS] = "NULL_SENDERS",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_SELECTOR] = "NULL_SELECTOR",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_INNER_SENDER] = "NULL_INNER_SENDER",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NO_SENDERS] = "NO_SENDERS",
        [SOLIDSYSLOG_SWITCHING_SENDER_ERROR_SELECTOR_OUT_OF_RANGE] = "SELECTOR_OUT_OF_RANGE",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX, detail);
}
