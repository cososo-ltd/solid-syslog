/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogUdpSenderErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const UdpSenderErrorsName_Names[] = {
    "NULL_CONFIG",
    "NULL_RESOLVER",
    "NULL_DATAGRAM",
    "NULL_ADDRESS",
    "NULL_ENDPOINT",
    "SEND_NULL_BUFFER",
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "DELIVERY_FAILED",
    "DELIVERY_RESTORED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(UdpSenderErrorsName_Names) / sizeof(UdpSenderErrorsName_Names[0])) == SOLIDSYSLOG_UDP_SENDER_ERROR_MAX,
    every_udp_sender_error_code_has_a_name
);

const char* SolidSyslogUdpSenderErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_UDP_SENDER_ERROR_MAX))
    {
        name = UdpSenderErrorsName_Names[detail];
    }
    return name;
}
