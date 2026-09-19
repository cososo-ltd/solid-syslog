/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGCMSISRTOSMUTEXPRIVATE_H
#define SOLIDSYSLOGCMSISRTOSMUTEXPRIVATE_H

#include <stdint.h>

#include "cmsis_os2.h"

#include "SolidSyslogCmsisRtosMutexErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"

/* osMutexNew returns an opaque id that need not be the control block it was
 * given - CMSIS-FreeRTOS sets the low bit for a recursive mutex - so the id is
 * stored rather than derived, and the control block stays the caller's. */
struct SolidSyslogCmsisRtosMutex
{
    struct SolidSyslogMutex Base;
    osMutexId_t Id;
};

void SolidSyslogCmsisRtosMutex_Initialise(
    struct SolidSyslogMutex* base,
    void* controlBlock,
    uint32_t controlBlockBytes
);
void SolidSyslogCmsisRtosMutex_Cleanup(struct SolidSyslogMutex* base);

static inline void CmsisRtosMutex_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMutexErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogCmsisRtosMutexErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGCMSISRTOSMUTEXPRIVATE_H */
