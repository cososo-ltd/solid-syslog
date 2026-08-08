#ifndef SOLIDSYSLOGWINDOWSHOSTNAMEINTERNAL_H
#define SOLIDSYSLOGWINDOWSHOSTNAMEINTERNAL_H

/* Library-internal test seam. Tests replace this function pointer via
   CppUTest's UT_PTR_SET to inject a fake hostname source (MSVC does not
   support GCC's weak/strong symbol override trick the fakes rely on). */

#include "SolidSyslogExternC.h"

#include <windows.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    typedef BOOL(WINAPI * WindowsGetComputerNameExAFn)(COMPUTER_NAME_FORMAT, LPSTR, LPDWORD);

    extern WindowsGetComputerNameExAFn WindowsHostname_GetComputerNameExA;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSHOSTNAMEINTERNAL_H */
