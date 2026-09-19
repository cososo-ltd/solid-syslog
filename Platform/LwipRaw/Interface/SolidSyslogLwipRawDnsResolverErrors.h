/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipRawDnsResolver adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogResolverErrors.h. */
#ifndef SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H
#define SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogResolverErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipRawDnsResolver. A handler matches by address
     *  (event->Source == &SolidSyslogLwipRawDnsResolverErrorSource), then reads event->Detail as an
     *  enum SolidSyslogResolverErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipRawDnsResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDNSRESOLVERERRORS_H */
