#ifndef SOLIDSYSLOGWINDOWSATOMICCOUNTER_H
#define SOLIDSYSLOGWINDOWSATOMICCOUNTER_H

#include "ExternC.h"
#include "SolidSyslogAtomicCounter.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_WINDOWSATOMICCOUNTER_SIZE = sizeof(intptr_t) * 2U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_WINDOWSATOMICCOUNTER_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogWindowsAtomicCounterStorage;

    struct SolidSyslogAtomicCounter* SolidSyslogWindowsAtomicCounter_Create(
        SolidSyslogWindowsAtomicCounterStorage * storage
    );
    void SolidSyslogWindowsAtomicCounter_Destroy(struct SolidSyslogAtomicCounter * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSATOMICCOUNTER_H */
