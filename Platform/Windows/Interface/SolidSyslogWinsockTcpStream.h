#ifndef SOLIDSYSLOGWINSOCKTCPSTREAM_H
#define SOLIDSYSLOGWINSOCKTCPSTREAM_H

#include "SolidSyslogStream.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"
#include "SolidSyslogTransport.h"

EXTERN_C_BEGIN

    struct SolidSyslogWinsockTcpStreamConfig
    {
        SolidSyslogTcpConnectTimeoutFunction
            GetConnectTimeoutMs; /* NULL → use SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable */
        void* ConnectTimeoutContext; /* passed through to GetConnectTimeoutMs; NULL is fine */
    };

    /* Precondition: caller has invoked WSAStartup() before using the stream,
       and will call WSACleanup() on shutdown. The library does not manage
       Winsock lifecycle. */
    struct SolidSyslogStream* SolidSyslogWinsockTcpStream_Create(const struct SolidSyslogWinsockTcpStreamConfig* config
    );
    void SolidSyslogWinsockTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAM_H */
