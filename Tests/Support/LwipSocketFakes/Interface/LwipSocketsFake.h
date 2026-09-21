#ifndef LWIPSOCKETSFAKE_H
#define LWIPSOCKETSFAKE_H

#include "SolidSyslogExternC.h"

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

SOLIDSYSLOG_EXTERN_C_END

#endif /* LWIPSOCKETSFAKE_H */
