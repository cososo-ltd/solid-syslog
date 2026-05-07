#include "SolidSyslogWindowsSleep.h"

#include <windows.h>

void SolidSyslogWindowsSleep(int milliseconds)
{
    Sleep((DWORD) milliseconds);
}
