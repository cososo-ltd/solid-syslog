#ifndef SOLIDSYSLOGPOSIXFILE_H
#define SOLIDSYSLOGPOSIXFILE_H

#include "ExternC.h"

struct SolidSyslogFile;

EXTERN_C_BEGIN

    struct SolidSyslogFile* SolidSyslogPosixFile_Create(void);
    void SolidSyslogPosixFile_Destroy(struct SolidSyslogFile * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXFILE_H */
