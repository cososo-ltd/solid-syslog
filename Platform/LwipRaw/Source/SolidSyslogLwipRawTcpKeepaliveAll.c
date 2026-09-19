/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* The variant for an lwIP built with LWIP_TCP_KEEPALIVE, where the probe
   interval and count are per-pcb fields, so all three timings are ours. */
#if LWIP_TCP && LWIP_TCP_KEEPALIVE

#include "lwip/tcp.h"

#include "SolidSyslogLwipRawTcpStreamPrivate.h"
#include "SolidSyslogTunables.h"

enum
{
    MILLISECONDS_PER_SECOND = 1000
};

void LwipRawTcpStream_ApplyKeepalive(struct tcp_pcb* pcb)
{
    pcb->keep_idle = (u32_t) SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS * MILLISECONDS_PER_SECOND;
    pcb->keep_intvl = (u32_t) SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS * MILLISECONDS_PER_SECOND;
    pcb->keep_cnt = (u32_t) SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT;
}

#endif /* LWIP_TCP && LWIP_TCP_KEEPALIVE */
