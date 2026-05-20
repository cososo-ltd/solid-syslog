#ifndef SOLIDSYSLOGWINDOWSFILE_H
#define SOLIDSYSLOGWINDOWSFILE_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogFile;

    struct SolidSyslogFile* SolidSyslogWindowsFile_Create(void);
    void SolidSyslogWindowsFile_Destroy(struct SolidSyslogFile * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSFILE_H */
