#include "SolidSyslogWindowsSleep.h"

#include <windows.h>

void SolidSyslogWindows_Sleep(int milliseconds)
{
    Sleep((DWORD) milliseconds);
}
