#ifndef FREERTOSARPFAKE_H
#define FREERTOSARPFAKE_H

#include "SolidSyslogExternC.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    void FreeRtosArpFake_Reset(void);

    void FreeRtosArpFake_SetCacheHit(bool hit);

    unsigned FreeRtosArpFake_IsIpInArpCacheCallCount(void);
    uint32_t FreeRtosArpFake_LastIsIpInArpCacheArg(void);

    unsigned FreeRtosArpFake_OutputArpRequestCallCount(void);
    uint32_t FreeRtosArpFake_LastOutputArpRequestArg(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* FREERTOSARPFAKE_H */
