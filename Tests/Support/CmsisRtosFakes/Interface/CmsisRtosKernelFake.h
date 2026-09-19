#ifndef CMSISRTOSKERNELFAKE_H
#define CMSISRTOSKERNELFAKE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void CmsisRtosKernelFake_Reset(void);

    void CmsisRtosKernelFake_SetTickCount(uint32_t ticks);

    void CmsisRtosKernelFake_SetTickFreq(uint32_t hertz);

    /* Whether the scheduler was locked at the moment the tick counter was
       read. The lock exists to make the read and the wrap count one step, so
       this is the claim worth asserting rather than a bare call count. */
    bool CmsisRtosKernelFake_WasLockedDuringTickRead(void);

    void CmsisRtosKernelFake_SetLocked(bool locked);

    bool CmsisRtosKernelFake_IsLocked(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* CMSISRTOSKERNELFAKE_H */
