#ifndef SOLIDSYSLOGFREERTOSMUTEX_H
#define SOLIDSYSLOGFREERTOSMUTEX_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(void);
    void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSMUTEX_H */
