#ifndef FILEFAKE_H
#define FILEFAKE_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;

SOLIDSYSLOG_EXTERN_C_BEGIN

    enum
    {
        /* Sized to fit struct FileFake on both ILP32 and LP64 hosts.
         * Bump if a new field grows the implementation past current capacity. */
        FILEFAKE_STORAGE_SLOTS = 15
    };

    struct FileFakeStorage
    {
        void* opaque[FILEFAKE_STORAGE_SLOTS];
    };

    struct SolidSyslogFile* FileFake_Create(struct FileFakeStorage * storage);
    void FileFake_Destroy(void);
    void FileFake_FailNextOpen(struct SolidSyslogFile * file);
    /* Lets @p opensThatSucceed opens through, then fails one. */
    void FileFake_FailOpenAfter(struct SolidSyslogFile * file, size_t opensThatSucceed);
    void FileFake_FailNextWrite(struct SolidSyslogFile * file);
    void FileFake_FailNextRead(struct SolidSyslogFile * file);
    /* Lets @p readsThatSucceed reads through, then fails one. */
    void FileFake_FailReadAfter(struct SolidSyslogFile * file, size_t readsThatSucceed);
    void FileFake_FailNextDelete(struct SolidSyslogFile * file);
    const void* FileFake_FileContent(void);
    size_t FileFake_FileSize(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* FILEFAKE_H */
