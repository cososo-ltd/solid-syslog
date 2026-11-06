#ifndef RAMFILEFAKE_H
#define RAMFILEFAKE_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;
    struct SolidSyslogFile;

    /* A File held in memory, one growing buffer per path, keeping the File
     * contract: Open creates a path or keeps its content, Read and Write move
     * one position, Size and Truncate act on the open file, Exists and Delete on
     * any path. Once failed it fails every call for good, as a broken file
     * system does, and reports each as a file pack does: FILE_IO_FAILED naming
     * the operation by its SolidSyslogFileErrors code, with the errno given.
     * Exists and Delete then answer false, reporting nothing, as the packs'
     * do. */
    struct SolidSyslogFile* RamFileFake_Create(void);
    void RamFileFake_Destroy(struct SolidSyslogFile * file);
    void RamFileFake_FailEveryOperation(struct SolidSyslogFile * file, int32_t nativeErrno);

    extern const struct SolidSyslogErrorSource RamFileFakeErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* RAMFILEFAKE_H */
