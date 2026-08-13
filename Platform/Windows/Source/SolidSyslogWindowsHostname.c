/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsHostname.h"
#include "SolidSyslogHeaderField.h"
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

void SolidSyslogWindows_GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    char hostname[MAX_HOSTNAME_SIZE];
    DWORD size = sizeof(hostname);

    (void) context;

    if (WindowsHostname_GetComputerNameExA(ComputerNamePhysicalDnsHostname, hostname, &size) != FALSE)
    {
        hostname[sizeof(hostname) - 1U] = '\0';
        SolidSyslogHeaderField_PrintUsAscii(field, hostname, sizeof(hostname));
    }
}
