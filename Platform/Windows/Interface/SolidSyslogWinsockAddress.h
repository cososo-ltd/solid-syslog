/** @file
 *  A Windows destination-address handle wrapping the Winsock struct sockaddr_in.
 *
 *  A Resolver writes the resolved IPv4 endpoint into it; a Datagram or Stream
 *  reads it back to send. It is a value slot the two sides share, not a vtable
 *  object. */
#ifndef SOLIDSYSLOGWINSOCKADDRESS_H
#define SOLIDSYSLOGWINSOCKADDRESS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    /** Draw one per sender from the pool (SOLIDSYSLOG_ADDRESS_POOL_SIZE); on
     *  exhaustion Create returns a shared TU-private singleton, so integrators
     *  that overflow the pool share that storage and race on it. */
    struct SolidSyslogAddress* SolidSyslogWinsockAddress_Create(void);
    /** Release the pool slot. */
    void SolidSyslogWinsockAddress_Destroy(struct SolidSyslogAddress * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKADDRESS_H */
