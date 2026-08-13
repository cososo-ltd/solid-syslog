/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The AtomicCounter vtable (Increment) - the contract an implementor fills in
 *  (the AtomicCounter extension point). */
#ifndef SOLIDSYSLOGATOMICCOUNTERDEFINITION_H
#define SOLIDSYSLOGATOMICCOUNTERDEFINITION_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Extension point for supplying the sequenceId counter (see
     *  SolidSyslogAtomicCounter_Increment for the caller-facing contract). An
     *  implementor is responsible for the wrap-aware [1, SOLIDSYSLOG_SEQUENCE_ID_MAX]
     *  range, for never yielding 0, and for making Increment safe under the
     *  concurrency its target platform allows. */
    struct SolidSyslogAtomicCounter
    {
        uint32_t (*Increment)(struct SolidSyslogAtomicCounter* base); /**< Advance and return the new value. */
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTERDEFINITION_H */
