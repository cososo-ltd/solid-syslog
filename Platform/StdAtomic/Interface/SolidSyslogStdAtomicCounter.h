/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An AtomicCounter over C11 <stdatomic.h>, backing the RFC 5424 sequenceId.
 *  Increment runs an atomic_compare_exchange_strong_explicit CAS loop
 *  on an _Atomic uint32_t; the sequence is wrap-aware in [1, 2^31 - 1] and
 *  skips zero on wrap, so a returned value is never 0. */
#ifndef SOLIDSYSLOGSTDATOMICCOUNTER_H
#define SOLIDSYSLOGSTDATOMICCOUNTER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullAtomicCounter, whose Increment returns 1 unconditionally. */
    struct SolidSyslogAtomicCounter* SolidSyslogStdAtomicCounter_Create(void);
    /** Release the pool slot; the counter's state is discarded. */
    void SolidSyslogStdAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTDATOMICCOUNTER_H */
