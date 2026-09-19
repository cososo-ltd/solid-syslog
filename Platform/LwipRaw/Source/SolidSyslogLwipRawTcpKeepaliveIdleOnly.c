/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* The variant for an lwIP built without LWIP_TCP_KEEPALIVE, where keep_intvl
   and keep_cnt are not fields of a pcb and the stack applies its own compile-
   time values for both. Only the idle period is ours to set. */
#if LWIP_TCP && !LWIP_TCP_KEEPALIVE

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
}

#endif /* LWIP_TCP && !LWIP_TCP_KEEPALIVE */
