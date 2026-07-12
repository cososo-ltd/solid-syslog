#ifndef SOLIDSYSLOGADDRESS_H
#define SOLIDSYSLOGADDRESS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /** Opaque resolved-destination handle. Obtained from a per-platform factory
     *  (SolidSyslog{Posix,Winsock,PlusTcp,LwipRaw}Address_Create); a Resolver
     *  writes the resolved address into it and a Datagram or Stream later reads
     *  it back to send. The concrete layout is platform-specific and private to
     *  that platform's sources. */
    struct SolidSyslogAddress;

EXTERN_C_END

#endif /* SOLIDSYSLOGADDRESS_H */
