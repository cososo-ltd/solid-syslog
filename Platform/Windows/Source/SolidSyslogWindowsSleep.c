/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsSleep.h"

#include <windows.h>

void SolidSyslogWindows_Sleep(int milliseconds)
{
    Sleep((DWORD) milliseconds);
}
