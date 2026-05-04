#ifndef SOLIDSYSLOGBLOCKDEVICEDEFINITION_H
#define SOLIDSYSLOGBLOCKDEVICEDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice
    {
        bool (*Acquire)(struct SolidSyslogBlockDevice* self, size_t blockIndex);
        bool (*Dispose)(struct SolidSyslogBlockDevice* self, size_t blockIndex);
        bool (*Exists)(struct SolidSyslogBlockDevice* self, size_t blockIndex);
        bool (*Read)(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, void* buf, size_t count);
        bool (*Append)(struct SolidSyslogBlockDevice* self, size_t blockIndex, const void* buf, size_t count);
        bool (*WriteAt)(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, const void* buf, size_t count);
        size_t (*Size)(struct SolidSyslogBlockDevice* self, size_t blockIndex);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKDEVICEDEFINITION_H */
