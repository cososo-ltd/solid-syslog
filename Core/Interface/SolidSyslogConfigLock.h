#ifndef SOLIDSYSLOGCONFIGLOCK_H
#define SOLIDSYSLOGCONFIGLOCK_H

#include "ExternC.h"

EXTERN_C_BEGIN

    typedef void (*SolidSyslogConfigLockFunction)(void);

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape
    void SolidSyslog_SetConfigLock(SolidSyslogConfigLockFunction lockFn, SolidSyslogConfigLockFunction unlockFn);
    void SolidSyslog_LockConfig(void);
    void SolidSyslog_UnlockConfig(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIGLOCK_H */
