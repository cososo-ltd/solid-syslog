#ifndef SOLIDSYSLOGPOSIXFILE_H
#define SOLIDSYSLOGPOSIXFILE_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogFile;

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_POSIX_FILE_SIZE = sizeof(intptr_t) * 11U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_POSIX_FILE_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogPosixFileStorage;

    struct SolidSyslogFile* SolidSyslogPosixFile_Create(SolidSyslogPosixFileStorage * storage);
    void SolidSyslogPosixFile_Destroy(struct SolidSyslogFile * file);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXFILE_H */
