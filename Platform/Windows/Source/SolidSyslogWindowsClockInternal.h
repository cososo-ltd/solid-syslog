#ifndef SOLIDSYSLOGWINDOWSCLOCKINTERNAL_H
#define SOLIDSYSLOGWINDOWSCLOCKINTERNAL_H

/* Library-internal test seam. Tests replace this function pointer via
   CppUTest's UT_PTR_SET to inject a fake FILETIME source (MSVC does not
   support GCC's weak/strong symbol override trick the fakes rely on). */

#include "SolidSyslogExternC.h"

#include <windows.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    typedef void(WINAPI * WindowsGetSystemTimeAsFileTimeFn)(LPFILETIME);

    extern WindowsGetSystemTimeAsFileTimeFn WindowsClock_GetSystemTimeAsFileTime;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSCLOCKINTERNAL_H */
