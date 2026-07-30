#ifndef DATAGRAMFAKE_H
#define DATAGRAMFAKE_H

#include <stddef.h>

#include "SolidSyslogExternC.h"
#include "SolidSyslogDatagram.h"

struct SolidSyslogDatagram;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    struct SolidSyslogDatagram* DatagramFake_Create(void);
    void DatagramFake_Destroy(struct SolidSyslogDatagram * datagram);

    /* Per-call SendTo result (0-based). Configured slots default to
     * SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT until overridden; SendTo
     * calls beyond DATAGRAMFAKE_MAX_SEND_CALLS return
     * SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED. */
    void DatagramFake_SetSendResult(
        struct SolidSyslogDatagram * datagram,
        int callIndex,
        enum SolidSyslogDatagramSendResult result
    );
    void DatagramFake_SetMaxPayload(struct SolidSyslogDatagram * datagram, size_t maxPayload);

    int DatagramFake_OpenCallCount(struct SolidSyslogDatagram * datagram);
    int DatagramFake_SendCallCount(struct SolidSyslogDatagram * datagram);
    int DatagramFake_MaxPayloadCallCount(struct SolidSyslogDatagram * datagram);
    int DatagramFake_CloseCallCount(struct SolidSyslogDatagram * datagram);
    const void* DatagramFake_SendBuffer(struct SolidSyslogDatagram * datagram, int callIndex);
    size_t DatagramFake_SendSize(struct SolidSyslogDatagram * datagram, int callIndex);

SOLIDSYSLOG_EXTERN_C_END

#endif /* DATAGRAMFAKE_H */
