#ifndef SOLIDSYSLOGMUTEXDEFINITION_H
#define SOLIDSYSLOGMUTEXDEFINITION_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex
    {
        void (*Lock)(struct SolidSyslogMutex* self);
        void (*Unlock)(struct SolidSyslogMutex* self);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGMUTEXDEFINITION_H */
