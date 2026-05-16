#ifndef SOLIDSYSLOGFILEDEFINITION_H
#define SOLIDSYSLOGFILEDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogFile
    {
        bool (*Open)(struct SolidSyslogFile* base, const char* path);
        void (*Close)(struct SolidSyslogFile* base);
        bool (*IsOpen)(struct SolidSyslogFile* base);
        bool (*Read)(struct SolidSyslogFile* base, void* buf, size_t count);
        bool (*Write)(struct SolidSyslogFile* base, const void* buf, size_t count);
        void (*SeekTo)(struct SolidSyslogFile* base, size_t offset);
        size_t (*Size)(struct SolidSyslogFile* base);
        void (*Truncate)(struct SolidSyslogFile* base);
        bool (*Exists)(struct SolidSyslogFile* base, const char* path);
        bool (*Delete)(struct SolidSyslogFile* base, const char* path);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGFILEDEFINITION_H */
