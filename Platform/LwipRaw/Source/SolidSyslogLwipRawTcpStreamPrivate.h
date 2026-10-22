/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLWIPRAWTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGLWIPRAWTCPSTREAMPRIVATE_H

#include <stdint.h>

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawTcpStream.h"
#include "SolidSyslogLwipRawTcpStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
#include "lwip/err.h"

struct tcp_pcb;
struct pbuf;

struct SolidSyslogLwipRawTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogLwipRawTcpStreamConfig Config;
    struct tcp_pcb* Pcb;
    bool Connected;
    bool Errored;
    /* The err_t behind Errored: the one the stack handed the connected or
       error callback, or ERR_CLSD for the close the receive callback saw.
       Written on the lwIP thread, and read there too, by a marshalled hop, so
       it reaches the caller's thread with Errored. */
    err_t LastError;
    struct pbuf* RxQueue[SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE];
    size_t RxQueueHead;
    size_t RxQueueCount;
    size_t RxHeadOffset;
};

void SolidSyslogLwipRawTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipRawTcpStreamConfig* config
);
void SolidSyslogLwipRawTcpStream_Cleanup(struct SolidSyslogStream* base);

/* Applies the SOLIDSYSLOG_TCP_KEEPALIVE_* tunables to a fresh pcb. Defined by
   whichever of the two keepalive translation units the lwIP build selects. */
void SolidSyslogLwipRawTcpStream_ApplyKeepalive(struct tcp_pcb* pcb);

static inline void LwipRawTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipRawTcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPRAWTCPSTREAMPRIVATE_H */
