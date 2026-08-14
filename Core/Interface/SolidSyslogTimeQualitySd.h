/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A StructuredData source for the RFC 5424 §7.1 "timeQuality" SD-ELEMENT (IANA
 *  SD-ID, so no enterprise-number suffix), emitted on every message the owning
 *  logger formats. The getTimeQuality callback is queried once per message; the
 *  element always carries tzKnown and isSynced (emitted as 0/1), and adds
 *  syncAccuracy when the clock is synced and the callback supplies an accuracy. */
#ifndef SOLIDSYSLOGTIMEQUALITYSD_H
#define SOLIDSYSLOGTIMEQUALITYSD_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimeQuality.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStructuredData;

    /** Create a "timeQuality" SD source (RFC 5424 §7.1), querying
     *  @p getTimeQuality once per message to build the element. Never returns
     *  NULL: a NULL callback or an exhausted pool reports via SolidSyslog_Error
     *  and returns the shared no-op NullSd, so callers need not null-check the
     *  result. */
    struct SolidSyslogStructuredData* SolidSyslogTimeQualitySd_Create(SolidSyslogTimeQualityFunction getTimeQuality);
    /** Release the SD's pool slot. */
    void SolidSyslogTimeQualitySd_Destroy(struct SolidSyslogStructuredData * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITYSD_H */
