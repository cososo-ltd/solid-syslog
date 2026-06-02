#include "BddTargetStderrErrorHandler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"

static void StderrErrorHandlerEx(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    const char* sourceName = "<unknown>";
    const char* message = "<no translation>";
    const struct SolidSyslogErrorSource* source = event->Source;
    if (source != NULL)
    {
        sourceName = source->Name;
        if (source->AsString != NULL)
        {
            message = source->AsString((uint8_t) event->Detail);
        }
    }
    if (event->Severity <= SOLIDSYSLOG_SEVERITY_ERROR)
    {
        (void) fprintf(
            stderr,
            "BDD-TARGET: FATAL: [%s cat=%u detail=%ld] %s\n",
            sourceName,
            (unsigned) event->Category,
            (long) event->Detail,
            message
        );
        (void) fflush(stderr);
        _Exit(3);
    }
    else
    {
        (void) fprintf(
            stderr,
            "[solidsyslog] severity=%d [%s cat=%u detail=%ld] %s\n",
            (int) event->Severity,
            sourceName,
            (unsigned) event->Category,
            (long) event->Detail,
            message
        );
    }
}

void BddTargetStderrErrorHandler_Install(void)
{
    SolidSyslog_SetErrorHandler(StderrErrorHandlerEx, NULL);
}
