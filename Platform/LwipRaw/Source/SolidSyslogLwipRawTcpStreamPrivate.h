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

struct tcp_pcb;
struct pbuf;

struct SolidSyslogLwipRawTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogLwipRawTcpStreamConfig Config;
    struct tcp_pcb* Pcb;
    bool Connected;
    bool Errored;
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

static inline void LwipRawTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogLwipRawTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipRawTcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPRAWTCPSTREAMPRIVATE_H */
