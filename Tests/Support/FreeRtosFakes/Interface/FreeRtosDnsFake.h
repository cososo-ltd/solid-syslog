#ifndef FREERTOSDNSFAKE_H
#define FREERTOSDNSFAKE_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"
#include "FreeRTOS.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void FreeRtosDnsFake_Reset(void);

    void FreeRtosDnsFake_SetGetAddrInfoFails(bool fails);

    unsigned FreeRtosDnsFake_GetAddrInfoCallCount(void);
    const char* FreeRtosDnsFake_LastGetAddrInfoHostname(void);
    BaseType_t FreeRtosDnsFake_LastGetAddrInfoSocktype(void);

    unsigned FreeRtosDnsFake_FreeAddrInfoCallCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* FREERTOSDNSFAKE_H */
