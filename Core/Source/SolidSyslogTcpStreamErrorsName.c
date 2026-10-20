/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTcpStreamErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the enum and indexed by its members, so a code added without a
 * name is a NULL the unit tests catch, and no name can shift. */
const char* SolidSyslogTcpStreamErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_TCP_STREAM_ERROR_MAX] = {
        [SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_NULL_CONFIG] = "NULL_CONFIG",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_NULL_SLEEP] = "NULL_SLEEP",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE] = "ENDPOINT_UNAVAILABLE",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_NOT_STARTED] = "CONNECT_NOT_STARTED",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED] = "CONNECT_REFUSED",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT] = "CONNECT_TIMED_OUT",
        [SOLIDSYSLOG_TCP_STREAM_ERROR_SOCKET_OPTION_REFUSED] = "SOCKET_OPTION_REFUSED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_TCP_STREAM_ERROR_MAX, detail);
}
