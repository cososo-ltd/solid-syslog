/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An AtomicCounter for a VxWorks 6.4 kernel (VIP) build, backing the RFC 5424
 *  sequenceId. Increment runs with interrupts locked (intLock), so it is safe
 *  to call from tasks and from interrupt service routines alike. That holds
 *  because VxWorks 6.4 runs on a single CPU: locking interrupts excludes every
 *  other writer. The sequence is wrap-aware in [1, 2^31 - 1] and skips zero on
 *  wrap, so a returned value is never 0. */
#ifndef SOLIDSYSLOGVXWORKS64ATOMICCOUNTER_H
#define SOLIDSYSLOGVXWORKS64ATOMICCOUNTER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullAtomicCounter, whose Increment returns 1 unconditionally, and is
     *  reported through the error handler. */
    struct SolidSyslogAtomicCounter* SolidSyslogVxWorks64AtomicCounter_Create(void);
    /** Release the pool slot; the counter's state is discarded. */
    void SolidSyslogVxWorks64AtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64ATOMICCOUNTER_H */
