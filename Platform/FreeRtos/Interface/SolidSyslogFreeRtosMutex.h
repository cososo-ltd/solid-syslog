#ifndef SOLIDSYSLOGFREERTOSMUTEX_H
#define SOLIDSYSLOGFREERTOSMUTEX_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    enum
    {
        SOLIDSYSLOG_FREERTOSMUTEX_SIZE = sizeof(intptr_t) * 20U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FREERTOSMUTEX_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogFreeRtosMutexStorage;

    struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(SolidSyslogFreeRtosMutexStorage * storage);
    void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex * mutex);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSMUTEX_H */
