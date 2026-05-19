#ifndef SOLIDSYSLOGPOSIXFILEPRIVATE_H
#define SOLIDSYSLOGPOSIXFILEPRIVATE_H

#include "SolidSyslogFileDefinition.h"

struct SolidSyslogPosixFile
{
    struct SolidSyslogFile Base;
    int Fd;
};

void PosixFile_Initialise(struct SolidSyslogFile* base);
void PosixFile_Cleanup(struct SolidSyslogFile* base);

#endif /* SOLIDSYSLOGPOSIXFILEPRIVATE_H */
