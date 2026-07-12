#ifndef SOLIDSYSLOGFILE_H
#define SOLIDSYSLOGFILE_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogFile;

    /** These dispatch straight to the SolidSyslogFile vtable; the per-operation
     *  contract (open-or-create, exact-count Read/Write, durable Write, seek and
     *  truncate semantics) lives in SolidSyslogFileDefinition.h. */
    bool SolidSyslogFile_Open(struct SolidSyslogFile * file, const char* path);
    void SolidSyslogFile_Close(struct SolidSyslogFile * file);
    bool SolidSyslogFile_IsOpen(struct SolidSyslogFile * file);
    bool SolidSyslogFile_Read(struct SolidSyslogFile * file, void* buf, size_t count);
    bool SolidSyslogFile_Write(struct SolidSyslogFile * file, const void* buf, size_t count);
    void SolidSyslogFile_SeekTo(struct SolidSyslogFile * file, size_t offset);
    size_t SolidSyslogFile_Size(struct SolidSyslogFile * file);
    void SolidSyslogFile_Truncate(struct SolidSyslogFile * file);
    bool SolidSyslogFile_Exists(struct SolidSyslogFile * file, const char* path);
    bool SolidSyslogFile_Delete(struct SolidSyslogFile * file, const char* path);

EXTERN_C_END

#endif /* SOLIDSYSLOGFILE_H */
