#include "ErrorHandlerFake.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"

enum
{
    ERRORHANDLERFAKE_KEPT_EVENTS = 8
};

static int handleCallCount;
static enum SolidSyslogSeverity lastSeverity;
static const struct SolidSyslogErrorSource* lastSource;
static uint16_t lastCategory;
static int32_t lastDetail;
static const void* lastContext;
static struct SolidSyslogErrorEvent keptEvents[ERRORHANDLERFAKE_KEPT_EVENTS];

static void Handle(void* context, const struct SolidSyslogErrorEvent* event)
{
    if (handleCallCount < ERRORHANDLERFAKE_KEPT_EVENTS)
    {
        keptEvents[handleCallCount] = *event;
    }
    handleCallCount++;
    lastSeverity = event->Severity;
    lastSource = event->Source;
    lastCategory = event->Category;
    lastDetail = event->Detail;
    lastContext = context;
}

void ErrorHandlerFake_Install(void* context)
{
    handleCallCount = 0;
    lastSeverity = SOLIDSYSLOG_SEVERITY_DEBUG;
    lastSource = NULL;
    lastCategory = 0U;
    lastDetail = 0;
    lastContext = NULL;
    for (int index = 0; index < ERRORHANDLERFAKE_KEPT_EVENTS; index++)
    {
        keptEvents[index] = (struct SolidSyslogErrorEvent) {SOLIDSYSLOG_SEVERITY_DEBUG, NULL, 0U, 0};
    }
    SolidSyslog_SetErrorHandler(Handle, context);
}

int ErrorHandlerFake_HandleCallCount(void)
{
    return handleCallCount;
}

enum SolidSyslogSeverity ErrorHandlerFake_LastSeverity(void)
{
    return lastSeverity;
}

const struct SolidSyslogErrorSource* ErrorHandlerFake_LastSource(void)
{
    return lastSource;
}

uint16_t ErrorHandlerFake_LastCategory(void)
{
    return lastCategory;
}

int32_t ErrorHandlerFake_LastDetail(void)
{
    return lastDetail;
}

const void* ErrorHandlerFake_LastContext(void)
{
    return lastContext;
}

static bool IsKept(int index)
{
    return (index >= 0) && (index < ERRORHANDLERFAKE_KEPT_EVENTS);
}

enum SolidSyslogSeverity ErrorHandlerFake_SeverityAt(int index)
{
    return IsKept(index) ? keptEvents[index].Severity : SOLIDSYSLOG_SEVERITY_DEBUG;
}

const struct SolidSyslogErrorSource* ErrorHandlerFake_SourceAt(int index)
{
    return IsKept(index) ? keptEvents[index].Source : NULL;
}

uint16_t ErrorHandlerFake_CategoryAt(int index)
{
    return IsKept(index) ? keptEvents[index].Category : 0U;
}

int32_t ErrorHandlerFake_DetailAt(int index)
{
    return IsKept(index) ? keptEvents[index].Detail : 0;
}
