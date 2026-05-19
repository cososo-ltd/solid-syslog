#include "BddTargetStderrErrorHandler.h"

#include <stdio.h>
#include <stdlib.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"

static void StderrErrorHandler(void* context, enum SolidSyslogSeverity severity, const char* message)
{
    (void) context;
    if (severity <= SOLIDSYSLOG_SEVERITY_ERROR)
    {
        (void) fprintf(stderr, "BDD-TARGET: FATAL: %s\n", message);
        (void) fflush(stderr);
        _Exit(3);
    }
    else
    {
        (void) fprintf(stderr, "[solidsyslog] severity=%d %s\n", (int) severity, message);
    }
}

void BddTargetStderrErrorHandler_Install(void)
{
    SolidSyslog_SetErrorHandler(StderrErrorHandler, NULL);
}
