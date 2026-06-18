#ifndef FREERTOSDNSFAKE_H
#define FREERTOSDNSFAKE_H

#include <stdbool.h>

#include "ExternC.h"
#include "FreeRTOS.h"

EXTERN_C_BEGIN

    void FreeRtosDnsFake_Reset(void);

    void FreeRtosDnsFake_SetGetAddrInfoFails(bool fails);

    unsigned FreeRtosDnsFake_GetAddrInfoCallCount(void);
    const char* FreeRtosDnsFake_LastGetAddrInfoHostname(void);
    BaseType_t FreeRtosDnsFake_LastGetAddrInfoSocktype(void);

    unsigned FreeRtosDnsFake_FreeAddrInfoCallCount(void);

EXTERN_C_END

#endif /* FREERTOSDNSFAKE_H */
