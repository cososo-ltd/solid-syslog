#ifndef SOLIDSYSLOGATOMICU32_H
#define SOLIDSYSLOGATOMICU32_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    typedef struct
    {
        intptr_t slot;
    } SolidSyslogAtomicU32Storage;

    struct SolidSyslogAtomicU32;

    struct SolidSyslogAtomicU32* SolidSyslogAtomicU32_FromStorage(SolidSyslogAtomicU32Storage * storage);
    void                         SolidSyslogAtomicU32_Init(struct SolidSyslogAtomicU32 * slot, uint32_t value);
    uint32_t                     SolidSyslogAtomicU32_Load(struct SolidSyslogAtomicU32 * slot);
    bool                         SolidSyslogAtomicU32_CompareAndSwap(struct SolidSyslogAtomicU32 * slot, uint32_t expected, uint32_t desired);

EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICU32_H */
