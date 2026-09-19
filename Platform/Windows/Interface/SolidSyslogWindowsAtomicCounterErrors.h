/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the WindowsAtomicCounter adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogAtomicCounterErrors.h. */
#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogAtomicCounterErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a WindowsAtomicCounter. A handler matches by address
     *  (event->Source == &SolidSyslogWindowsAtomicCounterErrorSource), then reads event->Detail as an
     *  enum SolidSyslogAtomicCounterErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogWindowsAtomicCounterErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTERERRORS_H */
