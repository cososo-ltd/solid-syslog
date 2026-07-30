/** @file
 *  A POSIX destination-address handle wrapping struct sockaddr_in.
 *
 *  A Resolver writes the resolved IPv4 endpoint into it; a Datagram or Stream
 *  reads it back to send. It is a value slot the two sides share, not a vtable
 *  object. */
#ifndef SOLIDSYSLOGPOSIXADDRESS_H
#define SOLIDSYSLOGPOSIXADDRESS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    /** Draw one per sender from the pool (SOLIDSYSLOG_ADDRESS_POOL_SIZE); on
     *  exhaustion Create returns a shared TU-private singleton, so integrators
     *  that overflow the pool share that storage and race on it. */
    struct SolidSyslogAddress* SolidSyslogPosixAddress_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPosixAddress_Destroy(struct SolidSyslogAddress * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXADDRESS_H */
