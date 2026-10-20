/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogUdpSenderErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const UdpSenderErrorsName_Names[] = {
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

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(UdpSenderErrorsName_Names) / sizeof(UdpSenderErrorsName_Names[0])) == SOLIDSYSLOG_UDP_SENDER_ERROR_MAX,
    every_udp_sender_error_code_has_a_name
);

const char* SolidSyslogUdpSenderErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(UdpSenderErrorsName_Names, (int32_t) SOLIDSYSLOG_UDP_SENDER_ERROR_MAX, detail);
}
