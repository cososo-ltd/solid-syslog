#ifndef MUTEXFAKE_H
#define MUTEXFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex* MutexFake_Create(void);
    void MutexFake_Destroy(void);
    const char* MutexFake_Sequence(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* MUTEXFAKE_H */
