#ifndef BDDTARGETSERVICETHREAD_H
#define BDDTARGETSERVICETHREAD_H

#include "ExternC.h"
#include "SolidSyslogSleep.h"

#include <stdbool.h>

EXTERN_C_BEGIN

    struct SolidSyslog;

    void BddTargetServiceThread_Run(
        struct SolidSyslog * handle,
        volatile bool* shutdown,
        SolidSyslogSleepFunction sleep
    );

EXTERN_C_END

#endif /* BDDTARGETSERVICETHREAD_H */
