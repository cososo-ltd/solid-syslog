/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable detail codes for the Address role, shared by every backend that fills it. */
#ifndef SOLIDSYSLOGADDRESSERRORS_H
#define SOLIDSYSLOGADDRESSERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Detail codes a Address role implementation reports through
     *  event->Detail, shared by every backend whichever platform provides it. A
     *  handler that matches on these reacts to a fault identically whichever
     *  backend produced it, and keeps working when the backend is swapped.
     *
     *  event->Source still names the backend that spoke, for diagnosis. Which of
     *  these codes a given backend can raise is a property of that backend and is
     *  stated on its own page; a code it never raises is simply one a handler
     *  never sees. */
    enum SolidSyslogAddressErrors
    {
        SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ADDRESS_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGADDRESSERRORS_H */
