/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSTDATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGSTDATOMICCOUNTERPRIVATE_H

#include <stdatomic.h>
#include <stdint.h>

#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStdAtomicCounterErrors.h"

struct SolidSyslogStdAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    _Atomic uint32_t Value;
};

void StdAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);
void StdAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base);

static inline void StdAtomicCounter_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogStdAtomicCounterErrors code
)
{
    SolidSyslog_Error(severity, &StdAtomicCounterErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGSTDATOMICCOUNTERPRIVATE_H */
