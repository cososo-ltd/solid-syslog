#ifndef SOLIDSYSLOGWINSOCKTCPSTREAMINTERNAL_H
#define SOLIDSYSLOGWINSOCKTCPSTREAMINTERNAL_H

/* Library-internal test seam. Tests replace these function pointers via
   CppUTest's UT_PTR_SET to inject fakes (MSVC does not support GCC's
   weak/strong symbol override trick used by the POSIX SocketFake).
   Names are namespaced WinsockTcpStream_* to avoid linker collisions
   with the un-namespaced Winsock_* symbols already exported by
   SolidSyslogWinsockDatagram and SolidSyslogWinsockResolver. */

#include "ExternC.h"

#include <winsock2.h>
#include <ws2tcpip.h>

EXTERN_C_BEGIN

    typedef SOCKET(WSAAPI * WinsockTcpStreamSocketFn)(int, int, int);
    typedef int(WSAAPI * WinsockTcpStreamConnectFn)(SOCKET, const struct sockaddr*, int);
    typedef int(WSAAPI * WinsockTcpStreamSendFn)(SOCKET, const char*, int, int);
    typedef int(WSAAPI * WinsockTcpStreamRecvFn)(SOCKET, char*, int, int);
    typedef int(WSAAPI * WinsockTcpStreamSetSockOptFn)(SOCKET, int, int, const char*, int);
    typedef int(WSAAPI * WinsockTcpStreamGetSockOptFn)(SOCKET, int, int, char*, int*);
    typedef int(WSAAPI * WinsockTcpStreamCloseSocketFn)(SOCKET);
    typedef int(WSAAPI * WinsockTcpStreamIoctlSocketFn)(SOCKET, long, u_long*);
    typedef int(WSAAPI * WinsockTcpStreamSelectFn)(int, fd_set*, fd_set*, fd_set*, const struct timeval*);
    typedef int(WSAAPI * WinsockTcpStreamWSAGetLastErrorFn)(void);

    extern WinsockTcpStreamSocketFn         WinsockTcpStream_socket;
    extern WinsockTcpStreamConnectFn        WinsockTcpStream_connect;
    extern WinsockTcpStreamSendFn           WinsockTcpStream_send;
    extern WinsockTcpStreamRecvFn           WinsockTcpStream_recv;
    extern WinsockTcpStreamSetSockOptFn     WinsockTcpStream_setsockopt;
    extern WinsockTcpStreamGetSockOptFn     WinsockTcpStream_getsockopt;
    extern WinsockTcpStreamCloseSocketFn    WinsockTcpStream_closesocket;
    extern WinsockTcpStreamIoctlSocketFn    WinsockTcpStream_ioctlsocket;
    extern WinsockTcpStreamSelectFn         WinsockTcpStream_select;
    extern WinsockTcpStreamWSAGetLastErrorFn WinsockTcpStream_WSAGetLastError;

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAMINTERNAL_H */
