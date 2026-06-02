#include "ErrorHandlerFake.h"

#include <stddef.h>

#include "SolidSyslogError.h"

static int handleCallCount;
static enum SolidSyslogSeverity lastSeverity;
static const struct SolidSyslogErrorSource* lastSource;
static uint16_t lastCategory;
static int32_t lastDetail;
static const void* lastContext;

static void Handle(void* context, const struct SolidSyslogErrorEvent* event)
{
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
