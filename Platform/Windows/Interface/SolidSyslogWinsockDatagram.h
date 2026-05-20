#ifndef SOLIDSYSLOGWINSOCKDATAGRAM_H
#define SOLIDSYSLOGWINSOCKDATAGRAM_H

#include "SolidSyslogDatagram.h"

EXTERN_C_BEGIN

    /* Precondition: caller has invoked WSAStartup() before using the datagram,
       and will call WSACleanup() on shutdown. The library does not manage
       Winsock lifecycle. */
    struct SolidSyslogDatagram* SolidSyslogWinsockDatagram_Create(void);
    void SolidSyslogWinsockDatagram_Destroy(struct SolidSyslogDatagram * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKDATAGRAM_H */
