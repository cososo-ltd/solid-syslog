#include "SolidSyslogWindowsProcessId.h"
#include "SolidSyslogFormatter.h"
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

void SolidSyslogWindowsProcessId_Get(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_Uint32(formatter, (uint32_t) WindowsProcessId_GetCurrentProcessId());
}
