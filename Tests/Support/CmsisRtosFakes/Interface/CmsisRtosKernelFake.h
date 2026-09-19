#ifndef CMSISRTOSKERNELFAKE_H
#define CMSISRTOSKERNELFAKE_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void CmsisRtosKernelFake_Reset(void);

    void CmsisRtosKernelFake_SetTickCount(uint32_t ticks);

SOLIDSYSLOG_EXTERN_C_END

#endif /* CMSISRTOSKERNELFAKE_H */
