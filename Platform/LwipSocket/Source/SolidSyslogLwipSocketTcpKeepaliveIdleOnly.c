/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* The variant for a stack built without LWIP_TCP_KEEPALIVE, where the probe
   interval and count are the stack's own compile-time values and only the idle
   period is ours. TCP_KEEPALIVE takes milliseconds. */
#if LWIP_SOCKET && LWIP_TCP && !LWIP_TCP_KEEPALIVE

#include "lwip/sockets.h"

#include <stdbool.h>

#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogTunables.h"

enum
{
    MILLISECONDS_PER_SECOND = 1000
};

bool SolidSyslogLwipSocketTcpStream_ApplyKeepalive(int fd)
{
    int idleMs = (int) SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS * MILLISECONDS_PER_SECOND;

    return lwip_setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idleMs, sizeof(idleMs)) == 0;
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpKeepaliveIdleOnly_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP && !LWIP_TCP_KEEPALIVE */
