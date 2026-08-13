/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsProcessId.h"
#include "SolidSyslogHeaderField.h"
#include "SolidSyslogWindowsProcessIdInternal.h"

#include <stdint.h>

/* File-local forwarder. Taking the address of an imported Windows API
   for static initialisation may trigger MSVC C4232 in some configurations;
   forwarding through a static function whose address IS a compile-time
   constant avoids the warning without a suppression. */
static DWORD WINAPI WindowsProcessId_CallGetCurrentProcessId(void);

WindowsGetCurrentProcessIdFn WindowsProcessId_GetCurrentProcessId = WindowsProcessId_CallGetCurrentProcessId;

static DWORD WINAPI WindowsProcessId_CallGetCurrentProcessId(void)
{
    return GetCurrentProcessId();
}

void SolidSyslogWindows_GetProcessId(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_Uint32(field, (uint32_t) WindowsProcessId_GetCurrentProcessId());
}
