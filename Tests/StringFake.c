#include "StringFake.h"

#include <string.h>

#include "SolidSyslogFormatter.h"

struct SolidSyslogFormatter;

static const char* fakeHostname;
static const char* fakeAppName;
static const char* fakeProcessId;

void StringFake_Reset(void)
{
    fakeHostname  = "";
    fakeAppName   = "";
    fakeProcessId = "";
}

void StringFake_SetHostname(const char* hostname)
{
    fakeHostname = hostname;
}

void StringFake_GetHostname(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, fakeHostname, strlen(fakeHostname));
}

void StringFake_SetAppName(const char* appName)
{
    fakeAppName = appName;
}

void StringFake_GetAppName(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, fakeAppName, strlen(fakeAppName));
}

void StringFake_SetProcessId(const char* procId)
{
    fakeProcessId = procId;
}

void StringFake_GetProcessId(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, fakeProcessId, strlen(fakeProcessId));
}
