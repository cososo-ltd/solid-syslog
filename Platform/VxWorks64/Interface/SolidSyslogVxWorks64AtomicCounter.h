/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An AtomicCounter for a VxWorks 6.4 kernel (VIP) build, backing the RFC 5424
 *  sequenceId. */
#ifndef SOLIDSYSLOGVXWORKS64ATOMICCOUNTER_H
#define SOLIDSYSLOGVXWORKS64ATOMICCOUNTER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    struct SolidSyslogAtomicCounter* SolidSyslogVxWorks64AtomicCounter_Create(void);
    void SolidSyslogVxWorks64AtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64ATOMICCOUNTER_H */
