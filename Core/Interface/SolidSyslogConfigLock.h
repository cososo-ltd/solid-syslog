/** @file
 *  The config-time lock injection pair guarding every pool Create/Destroy
 *  slot-walk; the no-op default suits single-task setup. */
#ifndef SOLIDSYSLOGCONFIGLOCK_H
#define SOLIDSYSLOGCONFIGLOCK_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Lock/unlock callback pair wrapping every pool Create/Destroy slot-walk,
     *  each passed the context installed with it. The default is a no-op, which
     *  suits single-task setup.
     *
     *  Where Create and Destroy can run concurrently, install a mutex supplied
     *  by the host system. Destroy releases the instance's resources inside
     *  this lock and may block, so the primitive must permit blocking while
     *  held; one that disables interrupts or spins will deadlock.
     *
     *  A SolidSyslog Mutex is not a valid choice: its Create walks a pool under
     *  this lock. Requiring a host primitive is also what allows the Mutex and
     *  AtomicCounter pools to use this pair for their own walks. */
    typedef void (*SolidSyslogConfigLockFunction)(void* context);

    /** Install the config-lock pair, applied setup-time before any Create.
     *  Single global slot, not synchronised against concurrent installs. All
     *  three are set together; @p context is passed to both callbacks
     *  unchanged, and NULL on either function restores that side's no-op
     *  default. */
    /* NOLINTBEGIN(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape */
    void SolidSyslog_SetConfigLock(
        SolidSyslogConfigLockFunction lockFn,
        SolidSyslogConfigLockFunction unlockFn,
        void* context
    );
    /* NOLINTEND(bugprone-easily-swappable-parameters) */
    void SolidSyslog_LockConfig(void);
    void SolidSyslog_UnlockConfig(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIGLOCK_H */
