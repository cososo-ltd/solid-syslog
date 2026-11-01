/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStreamOpenHealth.h"

#include "SolidSyslogError.h"

static inline bool StreamOpenHealth_IsNews(const struct SolidSyslogStreamOpenHealth* self, int32_t detail);
static inline void StreamOpenHealth_Latch(struct SolidSyslogStreamOpenHealth* self, int32_t detail);

void SolidSyslogStreamOpenHealth_ReportFailure(
    struct SolidSyslogStreamOpenHealth* self,
    const struct SolidSyslogErrorSource* source,
    enum SolidSyslogSeverity severity,
    uint16_t category,
    int32_t detail
)
{
    if (StreamOpenHealth_IsNews(self, detail))
    {
        StreamOpenHealth_Latch(self, detail);
        SolidSyslog_Error(severity, source, category, detail);
    }
}

/* A run is one detail repeating; the native code behind it may vary (a
 * different errno for the same refusal) without being news. */
static inline bool StreamOpenHealth_IsNews(const struct SolidSyslogStreamOpenHealth* self, int32_t detail)
{
    return !(self->Failing && (self->FailedDetail == detail));
}

static inline void StreamOpenHealth_Latch(struct SolidSyslogStreamOpenHealth* self, int32_t detail)
{
    self->Failing = true;
    self->FailedDetail = detail;
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
    if (StreamOpenHealth_IsNews(self, detail))
    {
        StreamOpenHealth_Latch(self, detail);
        SolidSyslog_ErrorWithNativeCode(severity, source, category, detail, nativeCode);
    }
}

void SolidSyslogStreamOpenHealth_Opened(struct SolidSyslogStreamOpenHealth* self)
{
    self->Failing = false;
}
