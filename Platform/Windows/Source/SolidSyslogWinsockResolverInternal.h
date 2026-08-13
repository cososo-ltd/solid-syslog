/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWINSOCKRESOLVERINTERNAL_H
#define SOLIDSYSLOGWINSOCKRESOLVERINTERNAL_H

/* Library-internal test seam. Tests replace these function pointers via
   CppUTest's UT_PTR_SET to inject fakes (MSVC does not support GCC's
   weak/strong symbol override trick the SocketFake relies on). */

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
