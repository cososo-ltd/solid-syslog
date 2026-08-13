/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the PosixFile adapter. */
#ifndef SOLIDSYSLOGPOSIXFILEERRORS_H
#define SOLIDSYSLOGPOSIXFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixFileErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPosixFileErrors
    {
        SOLIDSYSLOG_POSIX_FILE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_FILE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_FILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixFile. A handler matches by address
     *  (event->Source == &PosixFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogPosixFileErrors. */
    extern const struct SolidSyslogErrorSource PosixFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXFILEERRORS_H */
