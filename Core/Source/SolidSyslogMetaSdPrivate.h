/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGMETASDPRIVATE_H
#define SOLIDSYSLOGMETASDPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogMetaSdErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSdValueFunction.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogAtomicCounter;

struct SolidSyslogMetaSd
{
    struct SolidSyslogStructuredData Base;
    struct SolidSyslogAtomicCounter* Counter;
    SolidSyslogSysUpTimeFunction GetSysUpTime;
    SolidSyslogSdValueFunction GetLanguage;
    void* LanguageContext;
};

void MetaSd_Initialise(struct SolidSyslogStructuredData* base, const struct SolidSyslogMetaSdConfig* config);
void MetaSd_Cleanup(struct SolidSyslogStructuredData* base);

static inline void MetaSd_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMetaSdErrors code
)
{
    SolidSyslog_Error(severity, &MetaSdErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGMETASDPRIVATE_H */
