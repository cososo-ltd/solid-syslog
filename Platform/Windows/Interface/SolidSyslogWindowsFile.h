#ifndef SOLIDSYSLOGWINDOWSFILE_H
#define SOLIDSYSLOGWINDOWSFILE_H

#include "SolidSyslogFile.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_WINDOWS_FILE_SIZE = sizeof(intptr_t) * 11U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_WINDOWS_FILE_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogWindowsFileStorage;

    struct SolidSyslogFile* SolidSyslogWindowsFile_Create(SolidSyslogWindowsFileStorage * storage);
    void SolidSyslogWindowsFile_Destroy(struct SolidSyslogFile * file);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSFILE_H */
