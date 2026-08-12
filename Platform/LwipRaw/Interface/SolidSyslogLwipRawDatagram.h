/** @file
 *  UDP transport over the lwIP Raw API, for a UdpSender.
 *
 *  What the datagram does through its vtable is the substance:
 *
 *  - Open makes a udp_new pcb; Close removes it. Both run under the
 *    SolidSyslogLwipRaw_Marshal hop, since they touch lwIP core state.
 *  - SendTo runs the whole send - pbuf alloc, udp_sendto, free - in a single
 *    marshal hop, so a NO_SYS=0 integrator pays one tcpip-thread context switch
 *    per Send rather than three. The pbuf is PBUF_REF: lwIP points at the
 *    caller's buffer instead of copying it, safe because the buffer outlives the
 *    synchronous hop. Reports SENT on udp_sendto success, else FAILED. Cache-miss
 *    recovery is left to lwIP's ARP_QUEUEING.
 *  - MaxPayload returns the IPv6-safe default. */
#ifndef SOLIDSYSLOGLWIPRAWDATAGRAM_H
#define SOLIDSYSLOGLWIPRAWDATAGRAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullDatagram, whose SendTo reports SENT so undeliverables are dropped
     *  rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogLwipRawDatagram_Create(void);
    /** Release the pool slot; removes the udp pcb. */
    void SolidSyslogLwipRawDatagram_Destroy(struct SolidSyslogDatagram * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAM_H */
