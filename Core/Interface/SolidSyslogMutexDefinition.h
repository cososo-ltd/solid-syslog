#ifndef SOLIDSYSLOGMUTEXDEFINITION_H
#define SOLIDSYSLOGMUTEXDEFINITION_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex
    {
        void (*Lock)(struct SolidSyslogMutex* base);
        void (*Unlock)(struct SolidSyslogMutex* base);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGMUTEXDEFINITION_H */
