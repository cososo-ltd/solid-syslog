/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the PlusFatFile adapter. */
#ifndef SOLIDSYSLOGPLUSFATFILEERRORS_H
#define SOLIDSYSLOGPLUSFATFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PlusFatFileErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogPlusFatFileErrors
    {
        SOLIDSYSLOG_PLUSFAT_FILE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_PLUSFAT_FILE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_PLUSFAT_FILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusFatFile. A handler matches by address
     *  (event->Source == &PlusFatFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogPlusFatFileErrors. */
    extern const struct SolidSyslogErrorSource PlusFatFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSFATFILEERRORS_H */
