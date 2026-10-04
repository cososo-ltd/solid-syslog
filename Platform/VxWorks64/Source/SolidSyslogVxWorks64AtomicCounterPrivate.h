/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64ATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGVXWORKS64ATOMICCOUNTERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogVxWorks64AtomicCounterErrors.h"

struct SolidSyslogVxWorks64AtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    uint32_t Value;
};

void SolidSyslogVxWorks64AtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);
void SolidSyslogVxWorks64AtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base);

static inline void VxWorks64AtomicCounter_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogAtomicCounterErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64AtomicCounterErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64ATOMICCOUNTERPRIVATE_H */
