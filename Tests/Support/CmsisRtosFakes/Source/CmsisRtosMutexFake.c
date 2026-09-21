#include "CmsisRtosMutexFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cmsis_os2.h"

/* What osMutexNew hands back when it succeeds. A real implementation returns an
 * opaque id that need not be the control block - CMSIS-FreeRTOS sets the low
 * bit for a recursive mutex - so the fake returns something else entirely, and
 * an adapter that assumed the two were the same would fail its Destroy test. */
static uint8_t CmsisRtosMutexFake_Id = 0U;

static unsigned CmsisRtosMutexFake_NewCount = 0U;
static unsigned CmsisRtosMutexFake_AcquireCount = 0U;
static unsigned CmsisRtosMutexFake_ReleaseCount = 0U;
static unsigned CmsisRtosMutexFake_DeleteCount = 0U;

static void* CmsisRtosMutexFake_ControlBlock = NULL;
static uint32_t CmsisRtosMutexFake_ControlBlockBytes = 0U;
static uint32_t CmsisRtosMutexFake_AttrBits = 0U;
static bool CmsisRtosMutexFake_AttrSupplied = false;
static uint32_t CmsisRtosMutexFake_AcquireTimeout = 0U;
static osMutexId_t CmsisRtosMutexFake_CreatedId = NULL;
static osMutexId_t CmsisRtosMutexFake_DeletedId = NULL;
static bool CmsisRtosMutexFake_NewFails = false;

void CmsisRtosMutexFake_Reset(void)
{
    CmsisRtosMutexFake_NewCount = 0;
    CmsisRtosMutexFake_AcquireCount = 0;
    CmsisRtosMutexFake_ReleaseCount = 0;
    CmsisRtosMutexFake_DeleteCount = 0;
    CmsisRtosMutexFake_ControlBlock = NULL;
    CmsisRtosMutexFake_ControlBlockBytes = 0;
    CmsisRtosMutexFake_AttrBits = 0;
    CmsisRtosMutexFake_AttrSupplied = false;
    CmsisRtosMutexFake_AcquireTimeout = 0;
    CmsisRtosMutexFake_CreatedId = NULL;
    CmsisRtosMutexFake_DeletedId = NULL;
    CmsisRtosMutexFake_NewFails = false;
}

unsigned CmsisRtosMutexFake_MutexNewCallCount(void)
{
    return CmsisRtosMutexFake_NewCount;
}

unsigned CmsisRtosMutexFake_MutexAcquireCallCount(void)
{
    return CmsisRtosMutexFake_AcquireCount;
}

unsigned CmsisRtosMutexFake_MutexReleaseCallCount(void)
{
    return CmsisRtosMutexFake_ReleaseCount;
}

unsigned CmsisRtosMutexFake_MutexDeleteCallCount(void)
{
    return CmsisRtosMutexFake_DeleteCount;
}

void* CmsisRtosMutexFake_LastControlBlock(void)
{
    return CmsisRtosMutexFake_ControlBlock;
}

uint32_t CmsisRtosMutexFake_LastControlBlockBytes(void)
{
    return CmsisRtosMutexFake_ControlBlockBytes;
}

uint32_t CmsisRtosMutexFake_LastAttrBits(void)
{
    return CmsisRtosMutexFake_AttrBits;
}

bool CmsisRtosMutexFake_LastAttrWasSupplied(void)
{
    return CmsisRtosMutexFake_AttrSupplied;
}

uint32_t CmsisRtosMutexFake_LastAcquireTimeout(void)
{
    return CmsisRtosMutexFake_AcquireTimeout;
}

osMutexId_t CmsisRtosMutexFake_LastCreatedId(void)
{
    return CmsisRtosMutexFake_CreatedId;
}

osMutexId_t CmsisRtosMutexFake_LastDeletedId(void)
{
    return CmsisRtosMutexFake_DeletedId;
}

void CmsisRtosMutexFake_SetMutexNewFails(bool fails)
{
    CmsisRtosMutexFake_NewFails = fails;
}

osMutexId_t osMutexNew(const osMutexAttr_t* attr)
{
    osMutexId_t id = NULL;
    ++CmsisRtosMutexFake_NewCount;
    CmsisRtosMutexFake_AttrSupplied = (attr != NULL);
    if (attr != NULL)
    {
        CmsisRtosMutexFake_ControlBlock = attr->cb_mem;
        CmsisRtosMutexFake_ControlBlockBytes = attr->cb_size;
        CmsisRtosMutexFake_AttrBits = attr->attr_bits;
    }
    if (!CmsisRtosMutexFake_NewFails)
    {
        id = &CmsisRtosMutexFake_Id;
    }
    CmsisRtosMutexFake_CreatedId = id;
    return id;
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
    (void) mutex_id;
    ++CmsisRtosMutexFake_AcquireCount;
    CmsisRtosMutexFake_AcquireTimeout = timeout;
    return osOK;
}

osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
    (void) mutex_id;
    ++CmsisRtosMutexFake_ReleaseCount;
    return osOK;
}

osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
    ++CmsisRtosMutexFake_DeleteCount;
    CmsisRtosMutexFake_DeletedId = mutex_id;
    return osOK;
}
