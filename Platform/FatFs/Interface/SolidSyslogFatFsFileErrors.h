/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the FatFsFile adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogFileErrors.h. */
#ifndef SOLIDSYSLOGFATFSFILEERRORS_H
#define SOLIDSYSLOGFATFSFILEERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogFileErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a FatFsFile. A handler matches by address
     *  (event->Source == &SolidSyslogFatFsFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogFileErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogFatFsFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFATFSFILEERRORS_H */
