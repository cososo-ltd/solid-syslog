#ifndef SOLIDSYSLOGWINDOWSMUTEX_H
#define SOLIDSYSLOGWINDOWSMUTEX_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    enum
    {
        SOLIDSYSLOG_WINDOWSMUTEX_SIZE = sizeof(intptr_t) * 10
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_WINDOWSMUTEX_SIZE + sizeof(intptr_t) - 1) / sizeof(intptr_t)];
    } SolidSyslogWindowsMutexStorage;

    struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(SolidSyslogWindowsMutexStorage * storage);
    void                     SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex * mutex);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSMUTEX_H */
