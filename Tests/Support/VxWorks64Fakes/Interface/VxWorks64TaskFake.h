#ifndef VXWORKS64TASKFAKE_H
#define VXWORKS64TASKFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Records the task, clock, route and errno calls the BDD target makes. */
    void VxWorks64TaskFake_Reset(void);

    unsigned VxWorks64TaskFake_SpawnCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64TASKFAKE_H */
