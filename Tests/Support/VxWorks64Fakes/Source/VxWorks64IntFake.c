#include "vxWorks.h"

#include "VxWorks64IntFake.h"

#include "intLib.h"

#include <stddef.h>

static unsigned VxWorks64IntFake_LockCount = 0U;
static unsigned VxWorks64IntFake_UnlockCount = 0U;
static int VxWorks64IntFake_LockKey = 0;
static int VxWorks64IntFake_UnlockKey = 0;
static void (*VxWorks64IntFake_LockHook)(void) = NULL;
static void (*VxWorks64IntFake_UnlockHook)(void) = NULL;

void VxWorks64IntFake_Reset(void)
{
    VxWorks64IntFake_LockCount = 0U;
    VxWorks64IntFake_UnlockCount = 0U;
    VxWorks64IntFake_LockKey = 0;
    VxWorks64IntFake_UnlockKey = 0;
    VxWorks64IntFake_LockHook = NULL;
    VxWorks64IntFake_UnlockHook = NULL;
}

unsigned VxWorks64IntFake_IntLockCallCount(void)
{
    return VxWorks64IntFake_LockCount;
}

unsigned VxWorks64IntFake_IntUnlockCallCount(void)
{
    return VxWorks64IntFake_UnlockCount;
}

void VxWorks64IntFake_SetLockKey(int lockKey)
{
    VxWorks64IntFake_LockKey = lockKey;
}

int VxWorks64IntFake_LastUnlockKey(void)
{
    return VxWorks64IntFake_UnlockKey;
}

void VxWorks64IntFake_SetLockHook(void (*hook)(void))
{
    VxWorks64IntFake_LockHook = hook;
}

void VxWorks64IntFake_SetUnlockHook(void (*hook)(void))
{
    VxWorks64IntFake_UnlockHook = hook;
}

int intLock(void)
{
    VxWorks64IntFake_LockCount++;
    if (VxWorks64IntFake_LockHook != NULL)
    {
        VxWorks64IntFake_LockHook();
    }
    return VxWorks64IntFake_LockKey;
}

int intUnlock(int lockKey)
{
    VxWorks64IntFake_UnlockCount++;
    VxWorks64IntFake_UnlockKey = lockKey;
    if (VxWorks64IntFake_UnlockHook != NULL)
    {
        VxWorks64IntFake_UnlockHook();
    }
    return 0;
}
