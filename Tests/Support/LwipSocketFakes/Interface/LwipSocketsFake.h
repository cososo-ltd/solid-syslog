#ifndef LWIPSOCKETSFAKE_H
#define LWIPSOCKETSFAKE_H

#include "SolidSyslogExternC.h"

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

    /* lwip_sendto spy. The payload is copied, so a test reads what was sent
     * rather than the caller's buffer. */
    unsigned LwipSocketsFake_SendToCallCount(void);
    int LwipSocketsFake_LastSendToSocket(void);
    const void* LwipSocketsFake_LastSendToPayload(void);
    size_t LwipSocketsFake_LastSendToSize(void);
    int LwipSocketsFake_LastSendToFlags(void);
    const struct sockaddr_in* LwipSocketsFake_LastSendToAddress(void);
    socklen_t LwipSocketsFake_LastSendToAddressLength(void);

    /* lwip_close spy. */
    unsigned LwipSocketsFake_CloseCallCount(void);
    int LwipSocketsFake_LastClosedSocket(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LWIPSOCKETSFAKE_H */
