#ifndef VXWORKS64INTFAKE_H
#define VXWORKS64INTFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void VxWorks64IntFake_Reset(void);

    unsigned VxWorks64IntFake_IntLockCallCount(void);

    unsigned VxWorks64IntFake_IntUnlockCallCount(void);

    /** The key intLock hands back, standing in for the saved interrupt level. */
    void VxWorks64IntFake_SetLockKey(int lockKey);

    int VxWorks64IntFake_LastUnlockKey(void);

    /** Run hook inside intLock or intUnlock, standing in for an interrupt
     *  service routine that runs at the last moment before interrupts are
     *  locked, or the first moment after they are unlocked. A caller that
     *  touches shared state outside its critical section misses or loses
     *  what the hook does. NULL removes it. */
    void VxWorks64IntFake_SetLockHook(void (*hook)(void));

    void VxWorks64IntFake_SetUnlockHook(void (*hook)(void));

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64INTFAKE_H */
