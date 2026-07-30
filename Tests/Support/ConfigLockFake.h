#ifndef CONFIGLOCKFAKE_H
#define CONFIGLOCKFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void ConfigLockFake_Install(void);
    void ConfigLockFake_Uninstall(void);
    int ConfigLockFake_LockCallCount(void);
    int ConfigLockFake_UnlockCallCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* CONFIGLOCKFAKE_H */
