/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the PosixResolver adapter. */
#ifndef SOLIDSYSLOGPOSIXRESOLVERERRORS_H
#define SOLIDSYSLOGPOSIXRESOLVERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogPosixResolverErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPosixResolverErrors
    {
        SOLIDSYSLOG_POSIX_RESOLVER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_RESOLVER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_RESOLVER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PosixResolver. A handler matches by
     *  address (event->Source == &SolidSyslogPosixResolverErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPosixResolverErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPosixResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXRESOLVERERRORS_H */
