/** @file
 *  The config-time lock injection pair guarding every pool Create/Destroy
 *  slot-walk; the no-op default suits single-task setup. */
#ifndef SOLIDSYSLOGCONFIGLOCK_H
#define SOLIDSYSLOGCONFIGLOCK_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Lock/unlock callback pair wrapping every pool Create/Destroy slot-walk,
     *  each passed the context installed with it. Single-task setup needs none;
     *  the default is a no-op.
     *
     *  Where Create and Destroy can run concurrently, install a host mutex that
     *  may be held while blocking: Destroy releases the instance's resources
     *  inside this lock, and closing a transport or a file can block. A
     *  primitive that disables interrupts or spins deadlocks there.
     *
     *  Never a SolidSyslog Mutex — creating one walks a pool under this lock.
     *  Using the host's own primitive is also what lets the Mutex and
     *  AtomicCounter pools use this pair for their own walks. */
    typedef void (*SolidSyslogConfigLockFunction)(void* context);

    /** Install the config-lock pair, applied setup-time before any Create.
     *  Single global slot, not synchronised against concurrent installs. All
     *  three are set together; @p context is passed back to both callbacks
     *  unchanged, and NULL on either function restores that side's no-op
     *  default. */
    /* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape */
    void SolidSyslog_SetConfigLock(
        SolidSyslogConfigLockFunction lockFn,
        SolidSyslogConfigLockFunction unlockFn,
        void* context
    );
    void SolidSyslog_LockConfig(void);
    void SolidSyslog_UnlockConfig(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIGLOCK_H */
