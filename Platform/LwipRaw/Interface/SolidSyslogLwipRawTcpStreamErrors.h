/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the LwipRawTcpStream adapter. */
#ifndef SOLIDSYSLOGLWIPRAWTCPSTREAMERRORS_H
#define SOLIDSYSLOGLWIPRAWTCPSTREAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is LwipRawTcpStreamErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogLwipRawTcpStreamErrors
    {
        SOLIDSYSLOG_LWIPRAW_TCP_STREAM_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_LWIPRAW_TCP_STREAM_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_LWIPRAW_TCP_STREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a LwipRawTcpStream. A handler matches by
     *  address (event->Source == &LwipRawTcpStreamErrorSource), then reads
     *  event->Detail as an enum SolidSyslogLwipRawTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource LwipRawTcpStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWTCPSTREAMERRORS_H */
