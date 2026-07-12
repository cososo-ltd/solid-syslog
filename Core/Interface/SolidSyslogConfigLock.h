#ifndef SOLIDSYSLOGCONFIGLOCK_H
#define SOLIDSYSLOGCONFIGLOCK_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /** Critical-section enter/leave callback wrapping every pool Create/Destroy
     *  slot-walk. Single-task targets need none (the default is a no-op);
     *  multi-task targets wire taskENTER_CRITICAL (FreeRTOS),
     *  pthread_mutex_lock on a static mutex (POSIX), EnterCriticalSection
     *  (Windows), or a spinlock pair. Because this guards the pool walks, it is
     *  the one synchronisation primitive the Mutex and AtomicCounter pools can
     *  use for their own walks without a chicken-and-egg dependency on
     *  themselves. */
    typedef void (*SolidSyslogConfigLockFunction)(void);

    /** Install the config-lock pair, applied setup-time before any Create.
     *  Single global slot, not synchronised against concurrent installs. Both
     *  handlers are set together; NULL on either side restores that side's
     *  no-op default. */
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape
    void SolidSyslog_SetConfigLock(SolidSyslogConfigLockFunction lockFn, SolidSyslogConfigLockFunction unlockFn);
    void SolidSyslog_LockConfig(void);
    void SolidSyslog_UnlockConfig(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIGLOCK_H */
