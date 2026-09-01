/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H
#define SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H

#include <stdint.h>

#include <windows.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWindowsMutexErrors.h"

struct SolidSyslogWindowsMutex
{
    struct SolidSyslogMutex Base;
    CRITICAL_SECTION Section;
};

void SolidSyslogWindowsMutex_Initialise(struct SolidSyslogMutex* base);
void SolidSyslogWindowsMutex_Cleanup(struct SolidSyslogMutex* base);

static inline void WindowsMutex_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWindowsMutexErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWindowsMutexErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H */
