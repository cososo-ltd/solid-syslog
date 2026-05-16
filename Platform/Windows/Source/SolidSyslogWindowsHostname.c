#include "SolidSyslogWindowsHostname.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogWindowsHostnameInternal.h"

/* File-local forwarder. Taking the address of an imported Windows API
   for static initialisation may trigger MSVC C4232 in some configurations;
   forwarding through a static function whose address IS a compile-time
   constant avoids the warning without a suppression. */
static BOOL WINAPI WindowsHostname_CallGetComputerNameExA(COMPUTER_NAME_FORMAT nameType, LPSTR buffer, LPDWORD size);

WindowsGetComputerNameExAFn WindowsHostname_GetComputerNameExA = WindowsHostname_CallGetComputerNameExA;

static BOOL WINAPI WindowsHostname_CallGetComputerNameExA(COMPUTER_NAME_FORMAT nameType, LPSTR buffer, LPDWORD size)
{
    return GetComputerNameExA(nameType, buffer, size);
}

enum
{
    MAX_HOSTNAME_SIZE = 256U
};

void SolidSyslogWindowsHostname_Get(struct SolidSyslogFormatter* formatter)
{
    char hostname[MAX_HOSTNAME_SIZE];
    DWORD size = sizeof(hostname);

    if (WindowsHostname_GetComputerNameExA(ComputerNamePhysicalDnsHostname, hostname, &size) != FALSE)
    {
        hostname[sizeof(hostname) - 1U] = '\0';
        SolidSyslogFormatter_PrintUsAsciiString(formatter, hostname, sizeof(hostname));
    }
}
