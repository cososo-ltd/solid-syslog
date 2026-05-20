#ifndef SOLIDSYSLOGWINDOWSMUTEX_H
#define SOLIDSYSLOGWINDOWSMUTEX_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(void);
    void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSMUTEX_H */
