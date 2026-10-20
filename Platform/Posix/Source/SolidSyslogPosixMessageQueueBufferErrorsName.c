/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixMessageQueueBufferErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const PosixMessageQueueBufferErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "MQ_OPEN_FAILED",
    "SEND_FAILED",
    "RECEIVE_FAILED",
    "RECORD_REFUSED",
    "RECORDS_EVICTED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(PosixMessageQueueBufferErrorsName_Names) / sizeof(PosixMessageQueueBufferErrorsName_Names[0])) ==
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MAX,
    every_posix_message_queue_buffer_error_code_has_a_name
);

const char* SolidSyslogPosixMessageQueueBufferErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MAX))
    {
        name = PosixMessageQueueBufferErrorsName_Names[detail];
    }
    return name;
}
