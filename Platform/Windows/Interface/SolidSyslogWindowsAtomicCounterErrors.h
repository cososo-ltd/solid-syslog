/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the WindowsAtomicCounter adapter. */
#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogWindowsAtomicCounterErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogWindowsAtomicCounterErrors
    {
        SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WindowsAtomicCounter. A handler matches by
     *  address (event->Source == &SolidSyslogWindowsAtomicCounterErrorSource), then reads
     *  event->Detail as an enum SolidSyslogWindowsAtomicCounterErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogWindowsAtomicCounterErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H */
