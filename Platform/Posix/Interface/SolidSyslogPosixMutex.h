#ifndef SOLIDSYSLOGPOSIXMUTEX_H
#define SOLIDSYSLOGPOSIXMUTEX_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    enum
    {
        SOLIDSYSLOG_POSIXMUTEX_SIZE = sizeof(intptr_t) * 10U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_POSIXMUTEX_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogPosixMutexStorage;

    struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(SolidSyslogPosixMutexStorage * storage);
    void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex * mutex);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEX_H */
