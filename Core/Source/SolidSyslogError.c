/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogError.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogErrorCategory.h"

static void Error_NoOpErrorHandler(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    (void) event;
}

static SolidSyslogErrorHandler currentHandler = Error_NoOpErrorHandler;
static void* currentContext = NULL;

void SolidSyslog_SetErrorHandler(SolidSyslogErrorHandler handler, void* context)
{
    if (handler == NULL)
    {
        currentHandler = Error_NoOpErrorHandler;
    }
    else
    {
        currentHandler = handler;
    }
    currentContext = context;
}

static inline bool Error_EventHasNamedDetail(const struct SolidSyslogErrorEvent* event);

const char* SolidSyslogErrorEvent_DetailName(const struct SolidSyslogErrorEvent* event)
{
    const char* name = NULL;
    if (Error_EventHasNamedDetail(event))
    {
        name = event->Source->DetailName(event->Detail);
    }
    return name;
}

/* An integrator's own SolidSyslog_Error call may carry no source at all. */
static inline bool Error_EventHasNamedDetail(const struct SolidSyslogErrorEvent* event)
{
    return (event->Category != SOLIDSYSLOG_CAT_NATIVE_ERROR) && (event->Source != NULL) &&
           (event->Source->DetailName != NULL);
}

void SolidSyslog_Error(
    enum SolidSyslogSeverity severity,
    const struct SolidSyslogErrorSource* source,
    uint16_t category,
    int32_t detail
)
{
    SolidSyslog_ErrorWithNativeCode(severity, source, category, detail, 0);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- detail and nativeCode are both codes; every caller passes named values
void SolidSyslog_ErrorWithNativeCode(
    enum SolidSyslogSeverity severity,
    const struct SolidSyslogErrorSource* source,
    uint16_t category,
    int32_t detail,
    int32_t nativeCode
)
{
    const struct SolidSyslogErrorEvent event = {severity, source, category, detail, nativeCode};
    currentHandler(currentContext, &event);
}
