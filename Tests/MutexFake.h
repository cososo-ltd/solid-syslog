#ifndef MUTEXFAKE_H
#define MUTEXFAKE_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogMutex* MutexFake_Create(void);
    void                     MutexFake_Destroy(void);
    const char*              MutexFake_Sequence(void);

EXTERN_C_END

#endif /* MUTEXFAKE_H */
