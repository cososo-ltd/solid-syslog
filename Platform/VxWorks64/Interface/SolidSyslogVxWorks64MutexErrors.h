/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the VxWorks64Mutex adapter; the detail codes it reports
 *  are the portable ones in SolidSyslogMutexErrors.h. */
#ifndef SOLIDSYSLOGVXWORKS64MUTEXERRORS_H
#define SOLIDSYSLOGVXWORKS64MUTEXERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogMutexErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a VxWorks64Mutex. A handler matches by address
     *  (event->Source == &SolidSyslogVxWorks64MutexErrorSource), then reads event->Detail as an
     *  enum SolidSyslogMutexErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogVxWorks64MutexErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64MUTEXERRORS_H */
