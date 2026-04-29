#ifndef SOLIDSYSLOGWINDOWSSYSUPTIMEINTERNAL_H
#define SOLIDSYSLOGWINDOWSSYSUPTIMEINTERNAL_H

/* Library-internal test seam. Tests replace this function pointer via
   CppUTest's UT_PTR_SET to inject a fake tick source (MSVC does not
   support GCC's weak/strong symbol override trick used by the POSIX fakes). */

#include "ExternC.h"

#include <windows.h>

EXTERN_C_BEGIN

    typedef ULONGLONG(WINAPI * WindowsGetTickCount64Fn)(void);

    extern WindowsGetTickCount64Fn WindowsSysUpTime_GetTickCount64;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSYSUPTIMEINTERNAL_H */
