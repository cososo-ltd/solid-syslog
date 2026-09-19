#ifndef CMSISRTOSMUTEXFAKE_H
#define CMSISRTOSMUTEXFAKE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"
#include "cmsis_os2.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void CmsisRtosMutexFake_Reset(void);

    unsigned CmsisRtosMutexFake_MutexNewCallCount(void);

    unsigned CmsisRtosMutexFake_MutexAcquireCallCount(void);

    unsigned CmsisRtosMutexFake_MutexReleaseCallCount(void);

    unsigned CmsisRtosMutexFake_MutexDeleteCallCount(void);

    /** The attributes the adapter last handed osMutexNew, captured by value -
     *  CMSIS reads the struct only during the call, so the adapter is free to
     *  build it on the stack and the fake cannot keep the pointer. */
    void* CmsisRtosMutexFake_LastControlBlock(void);

    uint32_t CmsisRtosMutexFake_LastControlBlockBytes(void);

    uint32_t CmsisRtosMutexFake_LastAttrBits(void);

    bool CmsisRtosMutexFake_LastAttrWasSupplied(void);

    uint32_t CmsisRtosMutexFake_LastAcquireTimeout(void);

    /** The id osMutexNew last handed back, which is deliberately not the
     *  control block it was given. */
    osMutexId_t CmsisRtosMutexFake_LastCreatedId(void);

    osMutexId_t CmsisRtosMutexFake_LastDeletedId(void);

    /** Make osMutexNew hand back NULL, as a real implementation does when the
     *  control block it was given is smaller than the one it needs. */
    void CmsisRtosMutexFake_SetMutexNewFails(bool fails);

SOLIDSYSLOG_EXTERN_C_END

#endif /* CMSISRTOSMUTEXFAKE_H */
