/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An AtomicCounter over the Win32 Interlocked API, backing the RFC 5424
 *  sequenceId on Windows targets without C11 <stdatomic.h> (legacy MSVC).
 *  Increment runs a lock-free InterlockedCompareExchange CAS loop on a
 *  volatile LONG; the sequence is wrap-aware in [1, 2^31 - 1] and skips zero on
 *  wrap, so a returned value is never 0. */
#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTER_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullAtomicCounter, whose Increment returns 1 unconditionally. */
    struct SolidSyslogAtomicCounter* SolidSyslogWindowsAtomicCounter_Create(void);
    /** Release the pool slot; the counter's state is discarded. */
    void SolidSyslogWindowsAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTER_H */
