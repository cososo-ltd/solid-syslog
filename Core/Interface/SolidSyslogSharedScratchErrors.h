/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the SharedScratch. */
#ifndef SOLIDSYSLOGSHAREDSCRATCHERRORS_H
#define SOLIDSYSLOGSHAREDSCRATCHERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogSharedScratchErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogSharedScratchErrors
    {
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_NULL_MUTEX,
        SOLIDSYSLOG_SHARED_SCRATCH_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The detail's name, read beside the category's: NULL_MUTEX for
     *  SOLIDSYSLOG_SHARED_SCRATCH_ERROR_NULL_MUTEX. NULL for a code outside the enum. */
    const char* SolidSyslogSharedScratchErrors_Name(int32_t detail);

    /** The identity for events raised by a SharedScratch. A handler matches by
     *  address (event->Source == &SolidSyslogSharedScratchErrorSource), then
     *  reads event->Detail as an enum SolidSyslogSharedScratchErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogSharedScratchErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSHAREDSCRATCHERRORS_H */
