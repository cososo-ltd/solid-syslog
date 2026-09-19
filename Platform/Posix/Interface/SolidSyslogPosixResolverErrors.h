/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PosixResolver adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogResolverErrors.h. */
#ifndef SOLIDSYSLOGPOSIXRESOLVERERRORS_H
#define SOLIDSYSLOGPOSIXRESOLVERERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogResolverErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PosixResolver. A handler matches by address
     *  (event->Source == &SolidSyslogPosixResolverErrorSource), then reads event->Detail as an
     *  enum SolidSyslogResolverErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPosixResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXRESOLVERERRORS_H */
