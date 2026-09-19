/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipRawResolver adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogResolverErrors.h. */
#ifndef SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H
#define SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogResolverErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipRawResolver. A handler matches by address
     *  (event->Source == &SolidSyslogLwipRawResolverErrorSource), then reads event->Detail as an
     *  enum SolidSyslogResolverErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipRawResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H */
