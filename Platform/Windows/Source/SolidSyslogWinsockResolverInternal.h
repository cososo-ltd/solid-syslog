#ifndef SOLIDSYSLOGWINSOCKRESOLVERINTERNAL_H
#define SOLIDSYSLOGWINSOCKRESOLVERINTERNAL_H

/* Library-internal test seam. Tests replace these function pointers via
   CppUTest's UT_PTR_SET to inject fakes (MSVC does not support GCC's
   weak/strong symbol override trick used by the POSIX SocketFake). */

#include "SolidSyslogExternC.h"

#include <winsock2.h>
#include <ws2tcpip.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    typedef int(WSAAPI * WinsockGetAddrInfoFn)(const char*, const char*, const struct addrinfo*, struct addrinfo**);
    typedef void(WSAAPI * WinsockFreeAddrInfoFn)(struct addrinfo*);

    extern WinsockGetAddrInfoFn Winsock_getaddrinfo;
    extern WinsockFreeAddrInfoFn Winsock_freeaddrinfo;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKRESOLVERINTERNAL_H */
