#ifndef VXWORKS64SEMFAKE_H
#define VXWORKS64SEMFAKE_H

#include <stdbool.h>

#include "vxWorks.h"

#include "SolidSyslogExternC.h"
#include "semLib.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void VxWorks64SemFake_Reset(void);

    unsigned VxWorks64SemFake_SemMCreateCallCount(void);

    unsigned VxWorks64SemFake_SemTakeCallCount(void);

    unsigned VxWorks64SemFake_SemGiveCallCount(void);

    unsigned VxWorks64SemFake_SemDeleteCallCount(void);

    int VxWorks64SemFake_LastSemMCreateOptions(void);

    /** The id semMCreate last handed back. */
    SEM_ID VxWorks64SemFake_LastCreatedId(void);

    SEM_ID VxWorks64SemFake_LastTakenId(void);

    int VxWorks64SemFake_LastTakeTimeout(void);

    SEM_ID VxWorks64SemFake_LastGivenId(void);

    SEM_ID VxWorks64SemFake_LastDeletedId(void);

    /** Make semMCreate hand back NULL, as the kernel does when it cannot
     *  allocate the semaphore. */
    void VxWorks64SemFake_SetSemMCreateFails(bool fails);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64SEMFAKE_H */
