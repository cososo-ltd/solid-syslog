#ifndef SOLIDSYSLOGSTREAMDEFINITION_H
#define SOLIDSYSLOGSTREAMDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogStream.h"
#include "ExternC.h"

struct SolidSyslogAddress;

EXTERN_C_BEGIN

    struct SolidSyslogStream
    {
        bool (*Open)(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
        bool (*Send)(struct SolidSyslogStream* base, const void* buffer, size_t size);
        SolidSyslogSsize (*Read)(struct SolidSyslogStream* base, void* buffer, size_t size);
        void (*Close)(struct SolidSyslogStream* base);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAMDEFINITION_H */
