/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWINDOWSPROCESSIDINTERNAL_H
#define SOLIDSYSLOGWINDOWSPROCESSIDINTERNAL_H

/* Library-internal test seam. Tests replace this function pointer via
   CppUTest's UT_PTR_SET to inject a deterministic PID (MSVC does not
   support GCC's weak/strong symbol override trick the fakes rely on). */

#include "SolidSyslogExternC.h"

#include <windows.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    typedef DWORD(WINAPI * WindowsGetCurrentProcessIdFn)(void);

    extern WindowsGetCurrentProcessIdFn WindowsProcessId_GetCurrentProcessId;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSPROCESSIDINTERNAL_H */
