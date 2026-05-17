#ifndef SOLIDSYSLOGATOMICCOUNTERDEFINITION_H
#define SOLIDSYSLOGATOMICCOUNTERDEFINITION_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter
    {
        uint32_t (*Increment)(struct SolidSyslogAtomicCounter* base);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTERDEFINITION_H */
