/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the VxWorks64AtomicCounter adapter; the detail codes it
 *  reports are the portable ones in SolidSyslogAtomicCounterErrors.h. */
#ifndef SOLIDSYSLOGVXWORKS64ATOMICCOUNTERERRORS_H
#define SOLIDSYSLOGVXWORKS64ATOMICCOUNTERERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogAtomicCounterErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a VxWorks64AtomicCounter. A handler matches by
     *  address (event->Source == &SolidSyslogVxWorks64AtomicCounterErrorSource), then
     *  reads event->Detail as an enum SolidSyslogAtomicCounterErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogVxWorks64AtomicCounterErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64ATOMICCOUNTERERRORS_H */
