#include "SolidSyslogConfigLock.h"

#include <stddef.h>

static void ConfigLock_NoOp(void)
{
}

static SolidSyslogConfigLockFunction currentLock = ConfigLock_NoOp;
static SolidSyslogConfigLockFunction currentUnlock = ConfigLock_NoOp;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape
void SolidSyslog_SetConfigLock(SolidSyslogConfigLockFunction lockFn, SolidSyslogConfigLockFunction unlockFn)
{
    if (lockFn == NULL)
    {
        currentLock = ConfigLock_NoOp;
    }
    else
    {
        currentLock = lockFn;
    }
    if (unlockFn == NULL)
    {
        currentUnlock = ConfigLock_NoOp;
    }
    else
    {
        currentUnlock = unlockFn;
    }
}

void SolidSyslog_LockConfig(void)
{
    currentLock();
}

void SolidSyslog_UnlockConfig(void)
{
    currentUnlock();
}
