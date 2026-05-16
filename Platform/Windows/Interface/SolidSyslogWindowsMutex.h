#ifndef SOLIDSYSLOGWINDOWSMUTEX_H
#define SOLIDSYSLOGWINDOWSMUTEX_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    enum
    {
        SOLIDSYSLOG_WINDOWSMUTEX_SIZE = sizeof(intptr_t) * 10U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_WINDOWSMUTEX_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogWindowsMutexStorage;

    struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(SolidSyslogWindowsMutexStorage * storage);
    void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSMUTEX_H */
