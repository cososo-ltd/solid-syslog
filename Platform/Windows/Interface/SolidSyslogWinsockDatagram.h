/** @file
 *  UDP transport over a Winsock socket, for a UdpSender.
 *
 *  The datagram connect()s to the destination on its first SendTo (connected
 *  UDP) and turns on path-MTU discovery (IP_MTU_DISCOVER = IP_PMTUDISC_DO).
 *  SendTo then reports SENT, OVERSIZE (the datagram exceeds the path MTU,
 *  WSAEMSGSIZE), or FAILED. MaxPayload returns the IPv6-safe default until
 *  connected, then tracks the discovered path MTU (IP_MTU).
 *
 *  The caller must invoke WSAStartup before use and WSACleanup on shutdown; the
 *  library does not manage the Winsock lifecycle. */
#ifndef SOLIDSYSLOGWINSOCKDATAGRAM_H
#define SOLIDSYSLOGWINSOCKDATAGRAM_H

#include "SolidSyslogDatagram.h"

EXTERN_C_BEGIN

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullDatagram, whose SendTo reports SENT so undeliverables are dropped
     *  rather than backing up the Store. */
    struct SolidSyslogDatagram* SolidSyslogWinsockDatagram_Create(void);
    /** Release the pool slot and close the socket. */
    void SolidSyslogWinsockDatagram_Destroy(struct SolidSyslogDatagram * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKDATAGRAM_H */
