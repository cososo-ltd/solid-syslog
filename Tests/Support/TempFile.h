#ifndef TEMPFILE_H
#define TEMPFILE_H

/* Portable temporary file: MSVC bans tmpfile (C4996) and provides tmpfile_s,
   which is not available on other toolchains. CMake selects the platform-specific
   implementation (TempFileWindows.c or TempFileStandard.c). */

#include <stdio.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* A file open for update that is removed when closed, or NULL. */
    FILE* TempFile_Open(void);
    void TempFile_Close(FILE * file);

SOLIDSYSLOG_EXTERN_C_END

#endif /* TEMPFILE_H */
