/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable detail codes for the Resolver role, shared by every backend that fills it. */
#ifndef SOLIDSYSLOGRESOLVERERRORS_H
#define SOLIDSYSLOGRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Detail codes a Resolver role implementation reports through
     *  event->Detail, shared by every backend whichever platform provides it. A
     *  handler that matches on these reacts to a fault identically whichever
     *  backend produced it, and keeps working when the backend is swapped.
     *
     *  event->Source still names the backend that spoke, for diagnosis. Which of
     *  these codes a given backend can raise is a property of that backend and is
     *  stated on its own page; a code it never raises is simply one a handler
     *  never sees. */
    enum SolidSyslogResolverErrors
    {
        SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY,
        /** The bounded asynchronous-resolve wait hit its deadline. */
        SOLIDSYSLOG_RESOLVER_ERROR_RESOLVE_TIMEOUT,
        SOLIDSYSLOG_RESOLVER_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_RESOLVER_ERROR_NULL_SLEEP,
        /** The lookup could not answer in an address family the transports
         *  beside it can send to. Permanent for this destination: a stack or a
         *  destination offering only the other family will not start offering
         *  this one by being asked again. */
        SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED,
        SOLIDSYSLOG_RESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGRESOLVERERRORS_H */
