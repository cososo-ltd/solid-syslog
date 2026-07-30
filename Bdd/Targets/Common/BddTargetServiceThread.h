#ifndef BDDTARGETSERVICETHREAD_H
#define BDDTARGETSERVICETHREAD_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogSleep.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslog;

    void BddTargetServiceThread_Run(
        struct SolidSyslog * handle,
        volatile bool* shutdown,
        SolidSyslogSleepFunction sleep
    );

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETSERVICETHREAD_H */
