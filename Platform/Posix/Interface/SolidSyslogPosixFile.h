/** @file
 *  POSIX file I/O (open / read / write / lseek / ftruncate) behind the
 *  SolidSyslogFile vtable, for a file-backed BlockDevice or Store.
 *
 *  @ingroup platform_posix */
#ifndef SOLIDSYSLOGPOSIXFILE_H
#define SOLIDSYSLOGPOSIXFILE_H

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullFile. */
    struct SolidSyslogFile* SolidSyslogPosixFile_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPosixFile_Destroy(struct SolidSyslogFile * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXFILE_H */
