/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStreamOpenHealth.h"

#include "SolidSyslogError.h"

static inline bool StreamOpenHealth_Admit(struct SolidSyslogStreamOpenHealth* self, int32_t detail);

void SolidSyslogStreamOpenHealth_ReportFailure(
    struct SolidSyslogStreamOpenHealth* self,
    const struct SolidSyslogErrorSource* source,
    enum SolidSyslogSeverity severity,
    uint16_t category,
    int32_t detail
)
{
    if (StreamOpenHealth_Admit(self, detail))
    {
        SolidSyslog_Error(severity, source, category, detail);
    }
}

/* Whether this failure is news, latching it when it is. A run is one detail
 * repeating; the native code behind it may vary (a different errno for the
 * same refusal) without being news. */
static inline bool StreamOpenHealth_Admit(struct SolidSyslogStreamOpenHealth* self, int32_t detail)
{
    bool isNews = !(self->Failing && (self->FailedDetail == detail));
    if (isNews)
    {
        self->Failing = true;
        self->FailedDetail = detail;
    }
    return isNews;
}

void SolidSyslogStreamOpenHealth_ReportFailureWithNativeCode(
    struct SolidSyslogStreamOpenHealth* self,
    const struct SolidSyslogErrorSource* source,
    enum SolidSyslogSeverity severity,
    uint16_t category,
    int32_t detail,
    int32_t nativeCode
)
{
    if (StreamOpenHealth_Admit(self, detail))
    {
        SolidSyslog_ErrorWithNativeCode(severity, source, category, detail, nativeCode);
    }
}

void SolidSyslogStreamOpenHealth_Opened(struct SolidSyslogStreamOpenHealth* self)
{
    self->Failing = false;
}
