#include "StringFake.h"

#include <string.h>

#include "SolidSyslogHeaderField.h"

struct SolidSyslogHeaderField;

static const char* fakeHostname;
static const char* fakeAppName;
static const char* fakeProcessId;

void StringFake_Reset(void)
{
    fakeHostname = "";
    fakeAppName = "";
    fakeProcessId = "";
}

void StringFake_SetHostname(const char* hostname)
{
    fakeHostname = hostname;
}

void StringFake_GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, fakeHostname, strlen(fakeHostname));
}

void StringFake_SetAppName(const char* appName)
{
    fakeAppName = appName;
}

void StringFake_GetAppName(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, fakeAppName, strlen(fakeAppName));
}

void StringFake_SetProcessId(const char* procId)
{
    fakeProcessId = procId;
}

void StringFake_GetProcessId(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, fakeProcessId, strlen(fakeProcessId));
}
