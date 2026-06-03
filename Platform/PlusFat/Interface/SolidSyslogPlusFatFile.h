#ifndef SOLIDSYSLOGPLUSFATFILE_H
#define SOLIDSYSLOGPLUSFATFILE_H

#include "ExternC.h"

struct SolidSyslogFile;

EXTERN_C_BEGIN

    struct SolidSyslogFile* SolidSyslogPlusFatFile_Create(void);
    void SolidSyslogPlusFatFile_Destroy(struct SolidSyslogFile * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSFATFILE_H */
