#include "SolidSyslogConfigLock.h"

#include <stddef.h>

static void ConfigLock_NoOp(void)
{
}

static SolidSyslogConfigLockFunction ConfigLock_Lock = ConfigLock_NoOp;
static SolidSyslogConfigLockFunction ConfigLock_Unlock = ConfigLock_NoOp;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape
void SolidSyslog_SetConfigLock(SolidSyslogConfigLockFunction lockFn, SolidSyslogConfigLockFunction unlockFn)
{
    if (lockFn == NULL)
    {
        ConfigLock_Lock = ConfigLock_NoOp;
    }
    else
    {
        ConfigLock_Lock = lockFn;
    }
    if (unlockFn == NULL)
    {
        ConfigLock_Unlock = ConfigLock_NoOp;
    }
    else
    {
        ConfigLock_Unlock = unlockFn;
    }
}

void SolidSyslog_LockConfig(void)
{
    ConfigLock_Lock();
}

void SolidSyslog_UnlockConfig(void)
{
    ConfigLock_Unlock();
}
