#ifndef FILEFAKE_H
#define FILEFAKE_H

#include "SolidSyslogFile.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        FILEFAKE_STORAGE_SLOTS = 13
    };

    struct FileFakeStorage
    {
        void* opaque[FILEFAKE_STORAGE_SLOTS];
    };

    struct SolidSyslogFile* FileFake_Create(struct FileFakeStorage * storage);
    void                    FileFake_Destroy(void);
    void                    FileFake_FailNextOpen(struct SolidSyslogFile * file);
    void                    FileFake_FailNextWrite(struct SolidSyslogFile * file);
    void                    FileFake_FailNextRead(struct SolidSyslogFile * file);
    const void*             FileFake_FileContent(void);
    size_t                  FileFake_FileSize(void);

EXTERN_C_END

#endif /* FILEFAKE_H */
