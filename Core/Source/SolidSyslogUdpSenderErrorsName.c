/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogUdpSenderErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogUdpSenderErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_UDP_SENDER_ERROR_MAX] = {
        [SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_CONFIG] = "NULL_CONFIG",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_RESOLVER] = "NULL_RESOLVER",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_DATAGRAM] = "NULL_DATAGRAM",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_ADDRESS] = "NULL_ADDRESS",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_ENDPOINT] = "NULL_ENDPOINT",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_SEND_NULL_BUFFER] = "SEND_NULL_BUFFER",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_DELIVERY_FAILED] = "DELIVERY_FAILED",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_DELIVERY_RESTORED] = "DELIVERY_RESTORED",
        [SOLIDSYSLOG_UDP_SENDER_ERROR_RECORD_TOO_LARGE] = "RECORD_TOO_LARGE",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_UDP_SENDER_ERROR_MAX, detail);
}
