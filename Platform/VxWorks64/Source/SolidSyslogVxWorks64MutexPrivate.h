/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64MUTEXPRIVATE_H
#define SOLIDSYSLOGVXWORKS64MUTEXPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "vxWorks.h"

#include "semLib.h"

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogVxWorks64MutexErrors.h"

struct SolidSyslogVxWorks64Mutex
{
    struct SolidSyslogMutex Base;
    SEM_ID Id;
};

bool SolidSyslogVxWorks64Mutex_Initialise(struct SolidSyslogMutex* base);
void SolidSyslogVxWorks64Mutex_Cleanup(struct SolidSyslogMutex* base);

static inline void VxWorks64Mutex_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMutexErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64MutexErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64MUTEXPRIVATE_H */
