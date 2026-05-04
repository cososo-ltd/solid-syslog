#ifndef STRINGFAKE_H
#define STRINGFAKE_H

#include "ExternC.h"

struct SolidSyslogFormatter;

EXTERN_C_BEGIN

    void StringFake_Reset(void);
    void StringFake_SetHostname(const char* hostname);
    void StringFake_GetHostname(struct SolidSyslogFormatter * formatter);
    void StringFake_SetAppName(const char* appName);
    void StringFake_GetAppName(struct SolidSyslogFormatter * formatter);
    void StringFake_SetProcessId(const char* procId);
    void StringFake_GetProcessId(struct SolidSyslogFormatter * formatter);

EXTERN_C_END

#endif /* STRINGFAKE_H */
