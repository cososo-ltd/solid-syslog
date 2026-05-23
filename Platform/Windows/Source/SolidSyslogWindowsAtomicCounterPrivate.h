#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H

#include <stdint.h>
#include <windows.h>

#include "SolidSyslogAtomicCounterDefinition.h"

struct SolidSyslogWindowsAtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    volatile LONG Value;
};

void WindowsAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);
void WindowsAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base);

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTERPRIVATE_H */
