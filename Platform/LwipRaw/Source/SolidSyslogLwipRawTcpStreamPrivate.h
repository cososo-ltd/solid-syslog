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

void LwipRawTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipRawTcpStreamConfig* config
);
void LwipRawTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void LwipRawTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogLwipRawTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &LwipRawTcpStreamErrorSource, category, code);
}

#endif /* SOLIDSYSLOGLWIPRAWTCPSTREAMPRIVATE_H */
