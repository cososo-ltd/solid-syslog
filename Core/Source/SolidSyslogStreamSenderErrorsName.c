/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStreamSenderErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const StreamSenderErrorsName_Names[] = {
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_CONFIG] = "NULL_CONFIG",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_RESOLVER] = "NULL_RESOLVER",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_STREAM] = "NULL_STREAM",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_ADDRESS] = "NULL_ADDRESS",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_DELIVERY_FAILED] = "DELIVERY_FAILED",
    [SOLIDSYSLOG_STREAM_SENDER_ERROR_DELIVERY_RESTORED] = "DELIVERY_RESTORED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(StreamSenderErrorsName_Names) / sizeof(StreamSenderErrorsName_Names[0])) ==
        (size_t) SOLIDSYSLOG_STREAM_SENDER_ERROR_MAX,
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
