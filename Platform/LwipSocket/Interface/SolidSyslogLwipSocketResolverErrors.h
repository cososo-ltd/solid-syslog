/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipSocketResolver adapter; the detail codes it
 *  reports are the portable ones in SolidSyslogResolverErrors.h. */
#ifndef SOLIDSYSLOGLWIPSOCKETRESOLVERERRORS_H
#define SOLIDSYSLOGLWIPSOCKETRESOLVERERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogResolverErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipSocketResolver. A handler matches by
     *  address (event->Source == &SolidSyslogLwipSocketResolverErrorSource), then
     *  reads event->Detail as an enum SolidSyslogResolverErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipSocketResolverErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETRESOLVERERRORS_H */
