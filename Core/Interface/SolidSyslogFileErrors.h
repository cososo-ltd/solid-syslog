/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable detail codes for the File role, shared by every backend that fills it. */
#ifndef SOLIDSYSLOGFILEERRORS_H
#define SOLIDSYSLOGFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Detail codes a File role implementation reports through
     *  event->Detail, shared by every backend whichever platform provides it. A
     *  handler that matches on these reacts to a fault identically whichever
     *  backend produced it, and keeps working when the backend is swapped.
     *
     *  event->Source still names the backend that spoke, for diagnosis. Which of
     *  these codes a given backend can raise is a property of that backend and is
     *  stated on its own page; a code it never raises is simply one a handler
     *  never sees. */
    enum SolidSyslogFileErrors
    {
        SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY,
        /* Wiring a backend cannot work without. A backend that mounts its own
           filesystem, or needs no caller-supplied storage, never raises these. */
        SOLIDSYSLOG_FILE_ERROR_NULL_FILESYSTEM,
        SOLIDSYSLOG_FILE_ERROR_BUFFER_TOO_SMALL,
        SOLIDSYSLOG_FILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFILEERRORS_H */
