#include "vxWorks.h"

#include "VxWorks64SemFake.h"

#include "semLib.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* What semMCreate hands back when it succeeds: an address the adapter can only
 * store and pass back, standing in for the kernel's opaque semaphore. One per
 * pool slot would be truer, but each test reads only the most recent id. */
static uint8_t VxWorks64SemFake_Semaphore = 0U;

static unsigned VxWorks64SemFake_MCreateCount = 0U;
static unsigned VxWorks64SemFake_TakeCount = 0U;
static unsigned VxWorks64SemFake_GiveCount = 0U;
static unsigned VxWorks64SemFake_DeleteCount = 0U;
static int VxWorks64SemFake_MCreateOptions = 0;
static SEM_ID VxWorks64SemFake_CreatedId = NULL;
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
    VxWorks64SemFake_CreatedId = NULL;
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
    return VxWorks64SemFake_CreatedId;
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
    VxWorks64SemFake_CreatedId = VxWorks64SemFake_MCreateFails ? NULL : (SEM_ID) &VxWorks64SemFake_Semaphore;
    return VxWorks64SemFake_CreatedId;
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
