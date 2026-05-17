#ifndef SOLIDSYSLOGSTDATOMICCOUNTER_H
#define SOLIDSYSLOGSTDATOMICCOUNTER_H

#include "ExternC.h"
#include "SolidSyslogAtomicCounter.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_STDATOMICCOUNTER_SIZE = sizeof(intptr_t) * 2U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_STDATOMICCOUNTER_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogStdAtomicCounterStorage;

    struct SolidSyslogAtomicCounter* SolidSyslogStdAtomicCounter_Create(SolidSyslogStdAtomicCounterStorage * storage);
    void SolidSyslogStdAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGSTDATOMICCOUNTER_H */
