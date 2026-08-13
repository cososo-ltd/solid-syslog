/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogConfigLock.h"

#include <stddef.h>

static void ConfigLock_NoOp(void* context)
{
    (void) context;
}

static SolidSyslogConfigLockFunction ConfigLock_Lock = ConfigLock_NoOp;
static SolidSyslogConfigLockFunction ConfigLock_Unlock = ConfigLock_NoOp;
static void* ConfigLock_Context = NULL;

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- deliberate pair API: lock and unlock are installed together and conceptually inseparable; matches SolidSyslog_SetErrorHandler's pair shape
void SolidSyslog_SetConfigLock(
    SolidSyslogConfigLockFunction lockFn,
    SolidSyslogConfigLockFunction unlockFn,
    void* context
)
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    ConfigLock_Context = context;
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
    ConfigLock_Lock(ConfigLock_Context);
}

void SolidSyslog_UnlockConfig(void)
{
    ConfigLock_Unlock(ConfigLock_Context);
}
