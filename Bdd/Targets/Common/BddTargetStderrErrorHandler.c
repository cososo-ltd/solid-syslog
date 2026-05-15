#include "BddTargetStderrErrorHandler.h"

#include <stdio.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"

static void StderrErrorHandler(void* context, enum SolidSyslogSeverity severity, const char* message)
{
    (void) context;
    (void) fprintf(stderr, "[solidsyslog] severity=%d %s\n", (int) severity, message);
}

void BddTargetStderrErrorHandler_Install(void)
{
    SolidSyslog_SetErrorHandler(StderrErrorHandler, NULL);
}
