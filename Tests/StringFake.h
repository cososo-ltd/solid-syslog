#ifndef STRINGFAKE_H
#define STRINGFAKE_H

#include "ExternC.h"

struct SolidSyslogHeaderField;

EXTERN_C_BEGIN

    void StringFake_Reset(void);
    void StringFake_SetHostname(const char* hostname);
    void StringFake_GetHostname(struct SolidSyslogHeaderField * field, void* context);
    void StringFake_SetAppName(const char* appName);
    void StringFake_GetAppName(struct SolidSyslogHeaderField * field, void* context);
    void StringFake_SetProcessId(const char* procId);
    void StringFake_GetProcessId(struct SolidSyslogHeaderField * field, void* context);

EXTERN_C_END

#endif /* STRINGFAKE_H */
