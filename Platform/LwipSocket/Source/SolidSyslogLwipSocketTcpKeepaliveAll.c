/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* The variant for a stack built with LWIP_TCP_KEEPALIVE, where the probe
   interval and count are settable too, so all three timings are ours. These
   options take seconds, unlike the TCP_KEEPALIVE the other variant sets. */
#if LWIP_SOCKET && LWIP_TCP && LWIP_TCP_KEEPALIVE

#include "lwip/sockets.h"

#include <stdbool.h>

#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogTunables.h"

bool SolidSyslogLwipSocketTcpStream_ApplyKeepalive(int fd)
{
    int idleSeconds = (int) SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS;
    int intervalSeconds = (int) SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS;
    int probeCount = (int) SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT;

    bool accepted = (lwip_setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idleSeconds, sizeof(idleSeconds)) == 0);
    accepted =
        (lwip_setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intervalSeconds, sizeof(intervalSeconds)) == 0) && accepted;
    accepted = (lwip_setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &probeCount, sizeof(probeCount)) == 0) && accepted;
    return accepted;
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpKeepaliveAll_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP && LWIP_TCP_KEEPALIVE */
