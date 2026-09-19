/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PosixMutex adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogMutexErrors.h. */
#ifndef SOLIDSYSLOGPOSIXMUTEXERRORS_H
#define SOLIDSYSLOGPOSIXMUTEXERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogMutexErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PosixMutex. A handler matches by address
     *  (event->Source == &SolidSyslogPosixMutexErrorSource), then reads event->Detail as an
     *  enum SolidSyslogMutexErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPosixMutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEXERRORS_H */
