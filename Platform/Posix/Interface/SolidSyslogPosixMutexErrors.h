/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the PosixMutex adapter. */
#ifndef SOLIDSYSLOGPOSIXMUTEXERRORS_H
#define SOLIDSYSLOGPOSIXMUTEXERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixMutexErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPosixMutexErrors
    {
        SOLIDSYSLOG_POSIX_MUTEX_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_MUTEX_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_MUTEX_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixMutex. A handler matches by address
     *  (event->Source == &PosixMutexErrorSource), then reads event->Detail as an
     *  enum SolidSyslogPosixMutexErrors. */
    extern const struct SolidSyslogErrorSource PosixMutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEXERRORS_H */
