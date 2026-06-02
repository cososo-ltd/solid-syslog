#include "SolidSyslogError.h"

#include <stddef.h>

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

void SolidSyslog_Error(
    enum SolidSyslogSeverity severity,
    const struct SolidSyslogErrorSource* source,
    uint16_t category,
    int32_t detail
)
{
    const struct SolidSyslogErrorEvent event = {severity, source, category, detail};
    currentHandler(currentContext, &event);
}
