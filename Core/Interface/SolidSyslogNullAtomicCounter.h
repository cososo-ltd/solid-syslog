/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op AtomicCounter Null object: Increment returns 1U unconditionally, the safest
 *  value when a real counter is unavailable (RFC 5424 §7.3.1 forbids a sequenceId of 0). */
#ifndef SOLIDSYSLOGNULLATOMICCOUNTER_H
#define SOLIDSYSLOGNULLATOMICCOUNTER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Increment returns 1U unconditionally. RFC 5424 §7.3.1 forbids a sequenceId of 0,
     *  and 1U is indistinguishable from the post-power-on or post-wrap state, so it is the
     *  safest value when a real counter is unavailable. */
    struct SolidSyslogAtomicCounter* SolidSyslogNullAtomicCounter_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLATOMICCOUNTER_H */
