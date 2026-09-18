/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the FreeRtosMutex adapter. */
#ifndef SOLIDSYSLOGFREERTOSMUTEXERRORS_H
#define SOLIDSYSLOGFREERTOSMUTEXERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogFreeRtosMutexErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogFreeRtosMutexErrors
    {
        SOLIDSYSLOG_FREERTOS_MUTEX_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_FREERTOS_MUTEX_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FREERTOS_MUTEX_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a FreeRtosMutex. A handler matches by
     *  address (event->Source == &SolidSyslogFreeRtosMutexErrorSource), then reads
     *  event->Detail as an enum SolidSyslogFreeRtosMutexErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogFreeRtosMutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSMUTEXERRORS_H */
