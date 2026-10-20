/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStreamSenderErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const StreamSenderErrorsName_Names[] = {
    "NULL_CONFIG",
    "NULL_RESOLVER",
    "NULL_STREAM",
    "NULL_ADDRESS",
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "DELIVERY_FAILED",
    "DELIVERY_RESTORED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(StreamSenderErrorsName_Names) / sizeof(StreamSenderErrorsName_Names[0])) ==
        SOLIDSYSLOG_STREAM_SENDER_ERROR_MAX,
    every_stream_sender_error_code_has_a_name
);

const char* SolidSyslogStreamSenderErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        StreamSenderErrorsName_Names,
        (int32_t) SOLIDSYSLOG_STREAM_SENDER_ERROR_MAX,
        detail
    );
}
