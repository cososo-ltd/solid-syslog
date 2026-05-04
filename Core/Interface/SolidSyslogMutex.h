#ifndef SOLIDSYSLOGMUTEX_H
#define SOLIDSYSLOGMUTEX_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    void SolidSyslogMutex_Lock(struct SolidSyslogMutex * mutex);
    void SolidSyslogMutex_Unlock(struct SolidSyslogMutex * mutex);

EXTERN_C_END

#endif /* SOLIDSYSLOGMUTEX_H */
