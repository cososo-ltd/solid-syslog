#ifndef LWIPSOCKETSFAKE_H
#define LWIPSOCKETSFAKE_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stddef.h>

#include "lwip/sockets.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void LwipSocketsFake_Reset(void);

    /* The descriptor lwip_socket answers with. Defaults to a valid one; set a
     * negative value to make the stack refuse the socket. */
    void LwipSocketsFake_SetSocketResult(int result);

    /* lwip_socket spy. */
    unsigned LwipSocketsFake_SocketCallCount(void);
    int LwipSocketsFake_LastSocketDomain(void);
    int LwipSocketsFake_LastSocketType(void);
    int LwipSocketsFake_LastSocketProtocol(void);

    /* Makes the next lwip_sendto fail, returning -1 with this errno, the way
     * the sockets layer reports a refusal. */
    void LwipSocketsFake_SetSendToFailure(int err);

    /* lwip_sendto spy. The payload is copied, so a test reads what was sent
     * rather than the caller's buffer. */
    unsigned LwipSocketsFake_SendToCallCount(void);
    int LwipSocketsFake_LastSendToSocket(void);
    const void* LwipSocketsFake_LastSendToPayload(void);
    size_t LwipSocketsFake_LastSendToSize(void);
    int LwipSocketsFake_LastSendToFlags(void);
    const struct sockaddr_in* LwipSocketsFake_LastSendToAddress(void);
    socklen_t LwipSocketsFake_LastSendToAddressLength(void);

    /* What lwip_fcntl answers. Defaults to 0, the command accepted. */
    void LwipSocketsFake_SetFcntlResult(int result);

    /* lwip_fcntl spy. FcntlCallsBeforeConnect is the count as it stood when
     * lwip_connect was called, which is how a test pins the ordering. */
    unsigned LwipSocketsFake_FcntlCallCount(void);
    int LwipSocketsFake_LastFcntlSocket(void);
    int LwipSocketsFake_LastFcntlCommand(void);
    int LwipSocketsFake_LastFcntlValue(void);
    unsigned LwipSocketsFake_FcntlCallsBeforeConnect(void);

    /* Makes lwip_connect answer this, with this errno where it is a refusal.
     * Defaults to an immediate success. */
    void LwipSocketsFake_SetConnectResult(int result, int err);

    /* lwip_connect spy. */
    unsigned LwipSocketsFake_ConnectCallCount(void);
    int LwipSocketsFake_LastConnectSocket(void);
    const struct sockaddr_in* LwipSocketsFake_LastConnectAddress(void);
    socklen_t LwipSocketsFake_LastConnectAddressLength(void);

    /* What lwip_send answers. Defaults to the whole size written; set a short
     * count or -1 with an errno for a refusal. */
    void LwipSocketsFake_SetSendResult(ssize_t result, int err);

    /* lwip_send spy. The payload is copied, so a test reads what was sent. */
    unsigned LwipSocketsFake_SendCallCount(void);
    int LwipSocketsFake_LastSendSocket(void);
    const void* LwipSocketsFake_LastSendPayload(void);
    size_t LwipSocketsFake_LastSendSize(void);
    int LwipSocketsFake_LastSendFlags(void);

    /* What lwip_recv answers: the bytes it hands back, 0 for a peer close, or
     * -1 with an errno. Defaults to the text below, whose length is what a
     * successful read answers. */
    void LwipSocketsFake_SetRecvPayload(const char* payload);
    void LwipSocketsFake_SetRecvResult(ssize_t result, int err);

    /* lwip_recv spy. */
    unsigned LwipSocketsFake_RecvCallCount(void);
    int LwipSocketsFake_LastRecvSocket(void);
    size_t LwipSocketsFake_LastRecvSize(void);
    int LwipSocketsFake_LastRecvFlags(void);

    /* What lwip_select answers: the number of ready descriptors, or 0 for the
     * budget expiring, or -1. Defaults to one ready descriptor, and on a
     * positive answer the descriptor the caller watched for writing stays set.
     * LwipSocketsFake_SetSelectSignalsException puts it in the exception set
     * instead. */
    void LwipSocketsFake_SetSelectResult(int result);
    void LwipSocketsFake_SetSelectSignalsException(void);

    /* lwip_select spy. */
    unsigned LwipSocketsFake_SelectCallCount(void);
    int LwipSocketsFake_LastSelectMaxFdPlusOne(void);
    int LwipSocketsFake_LastSelectWriteDescriptor(void);
    int LwipSocketsFake_LastSelectExceptionDescriptor(void);
    unsigned LwipSocketsFake_LastSelectTimeoutMs(void);

    /* Makes lwip_setsockopt refuse this one option, the way a stack built
     * without support for it does. */
    void LwipSocketsFake_SetSockOptRefuses(int level, int optname);
    /* Makes lwip_setsockopt refuse every option it is given. */
    void LwipSocketsFake_SetSockOptRefusesEverything(void);

    /* lwip_setsockopt spy: whether the option was set, and to what. */
    unsigned LwipSocketsFake_SetSockOptCallCount(void);
    bool LwipSocketsFake_SockOptWasSetTo(int level, int optname, int value);

    /* The value lwip_getsockopt hands back for SO_ERROR. Defaults to 0, the
     * connect having completed. */
    void LwipSocketsFake_SetSocketError(int err);

    /* lwip_getsockopt spy. */
    unsigned LwipSocketsFake_GetSockOptCallCount(void);
    int LwipSocketsFake_LastGetSockOptSocket(void);
    int LwipSocketsFake_LastGetSockOptLevel(void);
    int LwipSocketsFake_LastGetSockOptName(void);

    /* lwip_close spy. */
    unsigned LwipSocketsFake_CloseCallCount(void);
    int LwipSocketsFake_LastClosedSocket(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LWIPSOCKETSFAKE_H */
