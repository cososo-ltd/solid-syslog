#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

/* Portable environment read: MSVC bans getenv (C4996) and provides getenv_s,
   which is not available on other toolchains. CMake selects the platform-specific
   implementation (EnvironmentWindows.c or EnvironmentStandard.c). */

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The variable's value, or NULL when it is not set. The pointer is to
       storage the next call may reuse, so copy what is needed. */
    const char* Environment_Get(const char* name);

SOLIDSYSLOG_EXTERN_C_END

#endif /* ENVIRONMENT_H */
