/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSHAREDSCRATCHPRIVATE_H
#define SOLIDSYSLOGSHAREDSCRATCHPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogSharedScratchErrors.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;

struct SolidSyslogSharedScratch
{
    struct SolidSyslogScratch Base;
    struct SolidSyslogMutex* Mutex;
    SolidSyslogFormatterStorage Storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
};

void SolidSyslogSharedScratch_Initialise(struct SolidSyslogScratch* base, struct SolidSyslogMutex* mutex);
void SolidSyslogSharedScratch_Cleanup(struct SolidSyslogScratch* base);

static inline void SharedScratch_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogSharedScratchErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogSharedScratchErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGSHAREDSCRATCHPRIVATE_H */
