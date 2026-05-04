#ifndef SOLIDSYSLOGBUFFERDEFINITION_H
#define SOLIDSYSLOGBUFFERDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer
    {
        void (*Write)(struct SolidSyslogBuffer* self, const void* data, size_t size);
        bool (*Read)(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGBUFFERDEFINITION_H */
