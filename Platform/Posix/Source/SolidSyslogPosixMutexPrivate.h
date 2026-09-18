/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPOSIXMUTEXPRIVATE_H
#define SOLIDSYSLOGPOSIXMUTEXPRIVATE_H

#include <stdint.h>

#include <pthread.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPosixMutexErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogPosixMutex
{
    struct SolidSyslogMutex Base;
    pthread_mutex_t Mutex;
};

void SolidSyslogPosixMutex_Initialise(struct SolidSyslogMutex* base);
void SolidSyslogPosixMutex_Cleanup(struct SolidSyslogMutex* base);

static inline void PosixMutex_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixMutexErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogPosixMutexErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXMUTEXPRIVATE_H */
