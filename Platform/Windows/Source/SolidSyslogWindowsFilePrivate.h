#ifndef SOLIDSYSLOGWINDOWSFILEPRIVATE_H
#define SOLIDSYSLOGWINDOWSFILEPRIVATE_H

#include "SolidSyslogFileDefinition.h"

struct SolidSyslogWindowsFile
{
    struct SolidSyslogFile Base;
    int Fd;
};

void WindowsFile_Initialise(struct SolidSyslogFile* base);
void WindowsFile_Cleanup(struct SolidSyslogFile* base);

#endif /* SOLIDSYSLOGWINDOWSFILEPRIVATE_H */
