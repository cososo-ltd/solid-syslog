#ifndef SOLIDSYSLOGPOSIXMUTEX_H
#define SOLIDSYSLOGPOSIXMUTEX_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(void);
    void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEX_H */
