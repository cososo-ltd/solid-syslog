#ifndef SOLIDSYSLOGFILEDEFINITION_H
#define SOLIDSYSLOGFILEDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogFile
    {
        bool (*Open)(struct SolidSyslogFile* self, const char* path);
        void (*Close)(struct SolidSyslogFile* self);
        bool (*IsOpen)(struct SolidSyslogFile* self);
        bool (*Read)(struct SolidSyslogFile* self, void* buf, size_t count);
        bool (*Write)(struct SolidSyslogFile* self, const void* buf, size_t count);
        void (*SeekTo)(struct SolidSyslogFile* self, size_t offset);
        size_t (*Size)(struct SolidSyslogFile* self);
        void (*Truncate)(struct SolidSyslogFile* self);
        bool (*Exists)(struct SolidSyslogFile* self, const char* path);
        bool (*Delete)(struct SolidSyslogFile* self, const char* path);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGFILEDEFINITION_H */
