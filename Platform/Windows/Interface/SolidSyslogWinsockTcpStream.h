#ifndef SOLIDSYSLOGWINSOCKTCPSTREAM_H
#define SOLIDSYSLOGWINSOCKTCPSTREAM_H

#include "SolidSyslogStream.h"
#include "SolidSyslogTransport.h"
#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_WINSOCK_TCP_STREAM_SIZE = sizeof(intptr_t) * 5U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_WINSOCK_TCP_STREAM_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogWinsockTcpStreamStorage;

    /* Precondition: caller has invoked WSAStartup() before using the stream,
       and will call WSACleanup() on shutdown. The library does not manage
       Winsock lifecycle. */
    struct SolidSyslogStream* SolidSyslogWinsockTcpStream_Create(SolidSyslogWinsockTcpStreamStorage * storage);
    void SolidSyslogWinsockTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAM_H */
