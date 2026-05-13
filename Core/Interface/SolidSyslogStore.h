#ifndef SOLIDSYSLOGSTORE_H
#define SOLIDSYSLOGSTORE_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogStore;

    bool   SolidSyslogStore_Write(struct SolidSyslogStore * store, const void* data, size_t size);
    bool   SolidSyslogStore_ReadNextUnsent(struct SolidSyslogStore * store, void* data, size_t maxSize, size_t* bytesRead);
    void   SolidSyslogStore_MarkSent(struct SolidSyslogStore * store);
    bool   SolidSyslogStore_HasUnsent(struct SolidSyslogStore * store);
    bool   SolidSyslogStore_IsHalted(struct SolidSyslogStore * store);
    size_t SolidSyslogStore_GetTotalBytes(struct SolidSyslogStore * store);
    size_t SolidSyslogStore_GetUsedBytes(struct SolidSyslogStore * store);
    bool   SolidSyslogStore_IsTransient(struct SolidSyslogStore * store);

EXTERN_C_END

#endif /* SOLIDSYSLOGSTORE_H */
