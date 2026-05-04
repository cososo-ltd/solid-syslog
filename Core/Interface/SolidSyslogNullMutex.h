#ifndef SOLIDSYSLOGNULLMUTEX_H
#define SOLIDSYSLOGNULLMUTEX_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    struct SolidSyslogMutex* SolidSyslogNullMutex_Create(void);
    void                     SolidSyslogNullMutex_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGNULLMUTEX_H */
