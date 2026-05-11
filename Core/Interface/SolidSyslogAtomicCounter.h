#ifndef SOLIDSYSLOGATOMICCOUNTER_H
#define SOLIDSYSLOGATOMICCOUNTER_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogAtomicCounter;

    struct SolidSyslogAtomicCounter* SolidSyslogAtomicCounter_Create(void);
    void                             SolidSyslogAtomicCounter_Destroy(void);
    uint32_t                         SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter * counter);

EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTER_H */
