/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixMessageQueueBufferErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogPosixMessageQueueBufferErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MAX] = {
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MQ_OPEN_FAILED] = "MQ_OPEN_FAILED",
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_SEND_FAILED] = "SEND_FAILED",
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECEIVE_FAILED] = "RECEIVE_FAILED",
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORD_REFUSED] = "RECORD_REFUSED",
        [SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORDS_EVICTED] = "RECORDS_EVICTED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MAX, detail);
}
