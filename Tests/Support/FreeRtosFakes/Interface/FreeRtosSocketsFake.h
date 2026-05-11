#ifndef FREERTOSSOCKETSFAKE_H
#define FREERTOSSOCKETSFAKE_H

#include "ExternC.h"

#include <stdbool.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

EXTERN_C_BEGIN

    void FreeRtosSocketsFake_Reset(void);

    /* socket configuration */
    void FreeRtosSocketsFake_SetSocketFails(bool fails);

    /* sendto configuration */
    void FreeRtosSocketsFake_SetSendtoFails(bool fails);

    /* connect configuration */
    void FreeRtosSocketsFake_SetConnectFails(bool fails);

    /* send configuration */
    void FreeRtosSocketsFake_SetSendFails(bool fails);        /* returns -pdFREERTOS_ERRNO_ENOTCONN */
    void FreeRtosSocketsFake_SetSendReturn(BaseType_t value); /* explicit return for short-write scenarios */

    /* recv configuration */
    void FreeRtosSocketsFake_SetRecvFails(bool fails);        /* returns -pdFREERTOS_ERRNO_ENOTCONN */
    void FreeRtosSocketsFake_SetRecvReturn(BaseType_t value); /* explicit return; 0 models a zero-timeout would-block */

    /* socket accessors */
    unsigned   FreeRtosSocketsFake_SocketCallCount(void);
    BaseType_t FreeRtosSocketsFake_LastSocketDomain(void);
    BaseType_t FreeRtosSocketsFake_LastSocketType(void);
    BaseType_t FreeRtosSocketsFake_LastSocketProtocol(void);
    Socket_t   FreeRtosSocketsFake_LastSocketReturned(void);

    /* sendto accessors */
    unsigned                        FreeRtosSocketsFake_SendtoCallCount(void);
    Socket_t                        FreeRtosSocketsFake_LastSendtoSocket(void);
    const void*                     FreeRtosSocketsFake_LastSendtoBuffer(void);
    size_t                          FreeRtosSocketsFake_LastSendtoLength(void);
    BaseType_t                      FreeRtosSocketsFake_LastSendtoFlags(void);
    const struct freertos_sockaddr* FreeRtosSocketsFake_LastSendtoDestination(void);
    socklen_t                       FreeRtosSocketsFake_LastSendtoDestinationLength(void);

    /* connect accessors */
    unsigned                        FreeRtosSocketsFake_ConnectCallCount(void);
    Socket_t                        FreeRtosSocketsFake_LastConnectSocket(void);
    const struct freertos_sockaddr* FreeRtosSocketsFake_LastConnectAddress(void);
    socklen_t                       FreeRtosSocketsFake_LastConnectAddressLength(void);
    /* Snapshot of SO_SNDTIMEO observed at the moment FreeRTOS_connect was
     * called — proves the bounded-connect contract (timeout set before
     * connect, cleared after). 0 if connect was never called. */
    TickType_t FreeRtosSocketsFake_SndTimeoAtConnect(void);

    /* send accessors */
    unsigned    FreeRtosSocketsFake_SendCallCount(void);
    Socket_t    FreeRtosSocketsFake_LastSendSocket(void);
    const void* FreeRtosSocketsFake_LastSendBuffer(void);
    size_t      FreeRtosSocketsFake_LastSendLength(void);
    BaseType_t  FreeRtosSocketsFake_LastSendFlags(void);

    /* recv accessors */
    unsigned   FreeRtosSocketsFake_RecvCallCount(void);
    Socket_t   FreeRtosSocketsFake_LastRecvSocket(void);
    void*      FreeRtosSocketsFake_LastRecvBuffer(void);
    size_t     FreeRtosSocketsFake_LastRecvLength(void);
    BaseType_t FreeRtosSocketsFake_LastRecvFlags(void);

    /* setsockopt accessors */
    TickType_t FreeRtosSocketsFake_LastSndTimeoSet(void);
    TickType_t FreeRtosSocketsFake_LastRcvTimeoSet(void);
    unsigned   FreeRtosSocketsFake_RcvTimeoSetCallCount(void);
    Socket_t   FreeRtosSocketsFake_LastSetsockoptSocket(void);
    int32_t    FreeRtosSocketsFake_LastSetsockoptLevel(void);
    int32_t    FreeRtosSocketsFake_LastSetsockoptOptionName(void);
    size_t     FreeRtosSocketsFake_LastSetsockoptOptionLength(void);

    /* closesocket accessors */
    unsigned FreeRtosSocketsFake_ClosesocketCallCount(void);
    Socket_t FreeRtosSocketsFake_LastClosesocketSocket(void);

EXTERN_C_END

#endif /* FREERTOSSOCKETSFAKE_H */
