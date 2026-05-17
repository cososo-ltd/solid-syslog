#include "SolidSyslogError.h"

#include <stddef.h>

static void Error_NoOpErrorHandler(void* context, enum SolidSyslogSeverity severity, const char* message)
{
    (void) context;
    (void) severity;
    (void) message;
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

void SolidSyslog_Error(enum SolidSyslogSeverity severity, const char* message)
{
    currentHandler(currentContext, severity, message);
}
