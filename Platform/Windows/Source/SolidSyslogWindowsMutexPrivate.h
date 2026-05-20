#ifndef SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H
#define SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H

#include <windows.h>

#include "SolidSyslogMutexDefinition.h"

struct SolidSyslogWindowsMutex
{
    struct SolidSyslogMutex Base;
    CRITICAL_SECTION Section;
};

void WindowsMutex_Initialise(struct SolidSyslogMutex* base);
void WindowsMutex_Cleanup(struct SolidSyslogMutex* base);

#endif /* SOLIDSYSLOGWINDOWSMUTEXPRIVATE_H */
