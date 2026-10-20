/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTcpStreamErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const TcpStreamErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
    "NULL_CONFIG",
    "NULL_SLEEP",
    "ENDPOINT_UNAVAILABLE",
    "CONNECT_NOT_STARTED",
    "CONNECT_REFUSED",
    "CONNECT_TIMED_OUT",
    "SOCKET_OPTION_REFUSED",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(TcpStreamErrorsName_Names) / sizeof(TcpStreamErrorsName_Names[0])) == SOLIDSYSLOG_TCP_STREAM_ERROR_MAX,
    every_tcp_stream_error_code_has_a_name
);

const char* SolidSyslogTcpStreamErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_TCP_STREAM_ERROR_MAX))
    {
        name = TcpStreamErrorsName_Names[detail];
    }
    return name;
}
