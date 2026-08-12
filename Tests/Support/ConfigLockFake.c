#include "ConfigLockFake.h"

#include <stddef.h>

#include "SolidSyslogConfigLock.h"

static int lockCallCount;
static int unlockCallCount;

static void Lock(void* context)
{
    (void) context;
    lockCallCount++;
}

static void Unlock(void* context)
{
    (void) context;
    unlockCallCount++;
}

void ConfigLockFake_Install(void)
{
    lockCallCount = 0;
    unlockCallCount = 0;
    SolidSyslog_SetConfigLock(Lock, Unlock, NULL);
}

void ConfigLockFake_Uninstall(void)
{
    SolidSyslog_SetConfigLock(NULL, NULL, NULL);
}

int ConfigLockFake_LockCallCount(void)
{
    return lockCallCount;
}

int ConfigLockFake_UnlockCallCount(void)
{
    return unlockCallCount;
}
