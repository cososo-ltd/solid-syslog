#ifndef FREERTOSDNSFAKE_H
#define FREERTOSDNSFAKE_H

#include "ExternC.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

EXTERN_C_BEGIN

    void FreeRtosDnsFake_Reset(void);

    /* getaddrinfo configuration */
    void FreeRtosDnsFake_SetGetAddrInfoFails(bool fails);

    /* getaddrinfo accessors */
    unsigned FreeRtosDnsFake_GetAddrInfoCallCount(void);
    const char* FreeRtosDnsFake_LastGetAddrInfoHostname(void);
    BaseType_t FreeRtosDnsFake_LastGetAddrInfoSocktype(void);

    /* freeaddrinfo accessors */
    unsigned FreeRtosDnsFake_FreeAddrInfoCallCount(void);

EXTERN_C_END

#endif /* FREERTOSDNSFAKE_H */
