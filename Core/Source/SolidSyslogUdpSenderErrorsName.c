/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogUdpSenderErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
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
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_UDP_SENDER_ERROR_MAX, detail);
}
