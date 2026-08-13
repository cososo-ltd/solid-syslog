/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWINSOCKDATAGRAMINTERNAL_H
#define SOLIDSYSLOGWINSOCKDATAGRAMINTERNAL_H

/* Library-internal test seam. Tests replace these function pointers via
   CppUTest's UT_PTR_SET to inject fakes (MSVC does not support GCC's
   weak/strong symbol override trick the SocketFake relies on). */

#include "SolidSyslogExternC.h"

#include <winsock2.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    typedef SOCKET(WSAAPI * WinsockSocketFn)(int, int, int);
    typedef int(WSAAPI * WinsockSendToFn)(SOCKET, const char*, int, int, const struct sockaddr*, int);
    typedef int(WSAAPI * WinsockConnectFn)(SOCKET, const struct sockaddr*, int);
    typedef int(WSAAPI * WinsockSetSockOptFn)(SOCKET, int, int, const char*, int);
    typedef int(WSAAPI * WinsockGetSockOptFn)(SOCKET, int, int, char*, int*);
    typedef int(WSAAPI * WinsockCloseSocketFn)(SOCKET);

    extern WinsockSocketFn Winsock_socket;
    extern WinsockSendToFn Winsock_sendto;
    extern WinsockConnectFn Winsock_connect;
    extern WinsockSetSockOptFn Winsock_setsockopt;
    extern WinsockGetSockOptFn Winsock_getsockopt;
    extern WinsockCloseSocketFn Winsock_closesocket;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKDATAGRAMINTERNAL_H */
