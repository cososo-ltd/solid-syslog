/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The atomic-counter role: hand out the next sequenceId (Increment), wrap-aware
 *  over [1, 2^31 - 1] and never 0 per RFC 5424 §7.3.1. This call dispatches to
 *  the injected counter's vtable, so behaviour is that counter's. */
#ifndef SOLIDSYSLOGATOMICCOUNTER_H
#define SOLIDSYSLOGATOMICCOUNTER_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_SEQUENCE_ID_MAX =
            2147483647U /* RFC 5424 §7.3.1: values in [1, 2^31 - 1], wraps to 1 on overflow. */
    };

    struct SolidSyslogAtomicCounter;

    /** Advance the counter and return its new value. Thread-safe: concurrent
     *  callers each get a distinct value across the [1, SOLIDSYSLOG_SEQUENCE_ID_MAX]
     *  cycle, so it can serve as the sequenceId of every message on a shared
     *  logger. The value is never 0 (RFC 5424 §7.3.1), so a returned 1 is
     *  ambiguous: it means either the first increment after power-on or the wrap
     *  past the maximum, and a reader cannot tell the two apart. */
    uint32_t SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTER_H */
