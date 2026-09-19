/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PlusFatFile adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogFileErrors.h. */
#ifndef SOLIDSYSLOGPLUSFATFILEERRORS_H
#define SOLIDSYSLOGPLUSFATFILEERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogFileErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PlusFatFile. A handler matches by address
     *  (event->Source == &SolidSyslogPlusFatFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogFileErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPlusFatFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSFATFILEERRORS_H */
