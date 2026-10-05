/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the VxWorks64File adapter; the detail codes it reports
 *  are the portable ones in SolidSyslogFileErrors.h. */
#ifndef SOLIDSYSLOGVXWORKS64FILEERRORS_H
#define SOLIDSYSLOGVXWORKS64FILEERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogFileErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a VxWorks64File. A handler matches by address
     *  (event->Source == &SolidSyslogVxWorks64FileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogFileErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogVxWorks64FileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64FILEERRORS_H */
