/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the FatFsFile adapter. */
#ifndef SOLIDSYSLOGFATFSFILEERRORS_H
#define SOLIDSYSLOGFATFSFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogFatFsFileErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogFatFsFileErrors
    {
        SOLIDSYSLOG_FATFS_FILE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_FATFS_FILE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FATFS_FILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a FatFsFile. A handler matches by address
     *  (event->Source == &SolidSyslogFatFsFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogFatFsFileErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogFatFsFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFATFSFILEERRORS_H */
