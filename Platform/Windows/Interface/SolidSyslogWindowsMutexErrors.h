/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the WindowsMutex adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogMutexErrors.h. */
#ifndef SOLIDSYSLOGWINDOWSMUTEXERRORS_H
#define SOLIDSYSLOGWINDOWSMUTEXERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogMutexErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a WindowsMutex. A handler matches by address
     *  (event->Source == &SolidSyslogWindowsMutexErrorSource), then reads event->Detail as an
     *  enum SolidSyslogMutexErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogWindowsMutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSMUTEXERRORS_H */
