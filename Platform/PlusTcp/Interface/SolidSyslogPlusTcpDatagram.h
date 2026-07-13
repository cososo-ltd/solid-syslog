/** @file
 *  UDP transport over a FreeRTOS-Plus-TCP socket, for a UdpSender.
 *
 *  SendTo wraps FreeRTOS_sendto (unconnected, destination per call) and reports
 *  SENT when the stack accepts the whole datagram, else FAILED. On an ARP-cache
 *  miss it first issues an ARP probe and yields once (~50 ms) for the reply,
 *  because FreeRTOS-Plus-TCP does not queue datagrams while ARP resolves — the
 *  cold-start packet would otherwise be dropped at the IP layer; if the reply
 *  is late the send is left to fail, since UDP is best-effort and retry belongs
 *  in the store-and-forward layer above. MaxPayload is the fixed IPv6-safe
 *  default. */
#ifndef SOLIDSYSLOGPLUSTCPDATAGRAM_H
#define SOLIDSYSLOGPLUSTCPDATAGRAM_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullDatagram, whose SendTo reports SENT so undeliverables are dropped
     *  rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogPlusTcpDatagram_Create(void);
    /** Release the pool slot and close the socket. */
    void SolidSyslogPlusTcpDatagram_Destroy(struct SolidSyslogDatagram * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPDATAGRAM_H */
