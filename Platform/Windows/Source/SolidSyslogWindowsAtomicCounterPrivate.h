/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H

#include <stdint.h>
#include <windows.h>

#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWindowsAtomicCounterErrors.h"

struct SolidSyslogWindowsAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    volatile LONG Value;
};

void SolidSyslogWindowsAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);
void SolidSyslogWindowsAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base);

static inline void WindowsAtomicCounter_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWindowsAtomicCounterErrors code
)
{
    SolidSyslog_Error(severity, &WindowsAtomicCounterErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H */
