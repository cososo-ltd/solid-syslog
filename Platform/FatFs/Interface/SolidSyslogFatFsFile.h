#ifndef SOLIDSYSLOGFATFSFILE_H
#define SOLIDSYSLOGFATFSFILE_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogFile;

EXTERN_C_BEGIN

    /* Sized for real FatFs with FF_MAX_SS=512 (99% case — SD/MMC, USB, HDD,
     * semihosting flat files) and FF_FS_TINY=0: 10-ptr base (~40 B) + FIL
     * (header ~56 B + 512 B sector buffer) + isOpen flag + alignment =
     * ~620 B. 180 intptrs gives ~100 B headroom on 32-bit. Integrators
     * who pick FF_MAX_SS > 512 hit the SOLIDSYSLOG_STATIC_ASSERT in
     * SolidSyslogFatFsFile.c at compile time; the planned S21.03 follow-up
     * makes this overridable via the tunables mechanism. */
    enum
    {
        SOLIDSYSLOG_FATFS_FILE_SIZE = sizeof(intptr_t) * 180U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FATFS_FILE_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogFatFsFileStorage;

    struct SolidSyslogFile* SolidSyslogFatFsFile_Create(SolidSyslogFatFsFileStorage * storage);
    void SolidSyslogFatFsFile_Destroy(struct SolidSyslogFile * file);

EXTERN_C_END

#endif /* SOLIDSYSLOGFATFSFILE_H */
