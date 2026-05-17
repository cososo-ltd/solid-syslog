#include "ErrorHandlerFake.h"

#include <stddef.h>

#include "SolidSyslogError.h"

static int handleCallCount;
static enum SolidSyslogSeverity lastSeverity;
static const char* lastMessage;
static const void* lastContext;

static void Handle(void* context, enum SolidSyslogSeverity severity, const char* message)
{
    handleCallCount++;
    lastSeverity = severity;
    lastMessage = message;
    lastContext = context;
}

void ErrorHandlerFake_Install(void* context)
{
    handleCallCount = 0;
    lastSeverity = SOLIDSYSLOG_SEVERITY_DEBUG;
    lastMessage = NULL;
    lastContext = NULL;
    SolidSyslog_SetErrorHandler(Handle, context);
}

void ErrorHandlerFake_Uninstall(void)
{
    SolidSyslog_SetErrorHandler(NULL, NULL);
}

int ErrorHandlerFake_HandleCallCount(void)
{
    return handleCallCount;
}

enum SolidSyslogSeverity ErrorHandlerFake_LastSeverity(void)
{
    return lastSeverity;
}

const char* ErrorHandlerFake_LastMessage(void)
{
    return lastMessage;
}

const void* ErrorHandlerFake_LastContext(void)
{
    return lastContext;
}
