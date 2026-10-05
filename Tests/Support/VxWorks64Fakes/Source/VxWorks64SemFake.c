#include "vxWorks.h"

#include "VxWorks64SemFake.h"

#include "semLib.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* What semMCreate hands back when it succeeds: an address the adapter can only
 * store and pass back, standing in for the kernel's opaque semaphore. Each call
 * has its own, so a test can tell two semaphores apart. */
enum
{
    VXWORKS64SEMFAKE_SEMAPHORES = 4
};

static uint8_t VxWorks64SemFake_Semaphores[VXWORKS64SEMFAKE_SEMAPHORES];

static unsigned VxWorks64SemFake_MCreateCount = 0U;
static unsigned VxWorks64SemFake_TakeCount = 0U;
static unsigned VxWorks64SemFake_GiveCount = 0U;
static unsigned VxWorks64SemFake_DeleteCount = 0U;
static int VxWorks64SemFake_MCreateOptions = 0;
static SEM_ID VxWorks64SemFake_LastCreated = NULL;
static SEM_ID VxWorks64SemFake_TakenId = NULL;
static int VxWorks64SemFake_TakeTimeout = 0;
static SEM_ID VxWorks64SemFake_GivenId = NULL;
static SEM_ID VxWorks64SemFake_DeletedId = NULL;
static bool VxWorks64SemFake_MCreateFails = false;

void VxWorks64SemFake_Reset(void)
{
    VxWorks64SemFake_MCreateCount = 0U;
    VxWorks64SemFake_TakeCount = 0U;
    VxWorks64SemFake_GiveCount = 0U;
    VxWorks64SemFake_DeleteCount = 0U;
    VxWorks64SemFake_MCreateOptions = 0;
    VxWorks64SemFake_LastCreated = NULL;
    VxWorks64SemFake_TakenId = NULL;
    VxWorks64SemFake_TakeTimeout = 0;
    VxWorks64SemFake_GivenId = NULL;
    VxWorks64SemFake_DeletedId = NULL;
    VxWorks64SemFake_MCreateFails = false;
}

unsigned VxWorks64SemFake_SemMCreateCallCount(void)
{
    return VxWorks64SemFake_MCreateCount;
}

unsigned VxWorks64SemFake_SemTakeCallCount(void)
{
    return VxWorks64SemFake_TakeCount;
}

unsigned VxWorks64SemFake_SemGiveCallCount(void)
{
    return VxWorks64SemFake_GiveCount;
}

unsigned VxWorks64SemFake_SemDeleteCallCount(void)
{
    return VxWorks64SemFake_DeleteCount;
}

int VxWorks64SemFake_LastSemMCreateOptions(void)
{
    return VxWorks64SemFake_MCreateOptions;
}

SEM_ID VxWorks64SemFake_LastCreatedId(void)
{
    return VxWorks64SemFake_LastCreated;
}

SEM_ID VxWorks64SemFake_CreatedId(unsigned call)
{
    return (call < VxWorks64SemFake_MCreateCount)
               ? (SEM_ID) &VxWorks64SemFake_Semaphores[call % VXWORKS64SEMFAKE_SEMAPHORES]
               : NULL;
}

SEM_ID VxWorks64SemFake_LastTakenId(void)
{
    return VxWorks64SemFake_TakenId;
}

int VxWorks64SemFake_LastTakeTimeout(void)
{
    return VxWorks64SemFake_TakeTimeout;
}

SEM_ID VxWorks64SemFake_LastGivenId(void)
{
    return VxWorks64SemFake_GivenId;
}

SEM_ID VxWorks64SemFake_LastDeletedId(void)
{
    return VxWorks64SemFake_DeletedId;
}

void VxWorks64SemFake_SetSemMCreateFails(bool fails)
{
    VxWorks64SemFake_MCreateFails = fails;
}

SEM_ID semMCreate(int options)
{
    VxWorks64SemFake_MCreateOptions = options;
    VxWorks64SemFake_MCreateCount++;
    VxWorks64SemFake_LastCreated =
        VxWorks64SemFake_MCreateFails
            ? NULL
            : (SEM_ID) &VxWorks64SemFake_Semaphores[(VxWorks64SemFake_MCreateCount - 1U) % VXWORKS64SEMFAKE_SEMAPHORES];
    return VxWorks64SemFake_LastCreated;
}

STATUS semTake(SEM_ID semId, int timeout)
{
    VxWorks64SemFake_TakeCount++;
    VxWorks64SemFake_TakenId = semId;
    VxWorks64SemFake_TakeTimeout = timeout;
    return OK;
}

STATUS semGive(SEM_ID semId)
{
    VxWorks64SemFake_GiveCount++;
    VxWorks64SemFake_GivenId = semId;
    return OK;
}

STATUS semDelete(SEM_ID semId)
{
    VxWorks64SemFake_DeleteCount++;
    VxWorks64SemFake_DeletedId = semId;
    return OK;
}
