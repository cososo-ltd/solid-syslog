/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The library's own structured-data element, carried by each report Service
 * makes of a pending event. Built on Service's stack for that one report and
 * formatted as the record's per-message SD, so it has no pool (D.002). */
#ifndef SOLIDSYSLOGEVENTSD_H
#define SOLIDSYSLOGEVENTSD_H

#include "SolidSyslogError.h"
#include "SolidSyslogExternC.h"
#include "SolidSyslogStructuredDataDefinition.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogEventSd
    {
        struct SolidSyslogStructuredData Base;
        const struct SolidSyslogErrorEvent* Event;
    };

    /* Readies @p self to format @p event, which must outlive the format, and
     * returns it as the StructuredData to hand the message formatter. */
    struct SolidSyslogStructuredData* SolidSyslogEventSd_Initialise(
        struct SolidSyslogEventSd * self,
        const struct SolidSyslogErrorEvent* event
    );

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGEVENTSD_H */
