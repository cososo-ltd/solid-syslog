#ifndef VXWORKS64NETFAKE_H
#define VXWORKS64NETFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void VxWorks64NetFake_Reset(void);

    /** The value inet_addr answers; the address in network byte order, or
     *  (unsigned long) ERROR for a string that is not a dotted literal. */
    void VxWorks64NetFake_SetInetAddrReturn(unsigned long value);

    const char* VxWorks64NetFake_LastInetAddrString(void);

    /** The value hostGetByName answers; the address in network byte order, or
     *  ERROR for a name it cannot resolve. */
    void VxWorks64NetFake_SetHostGetByNameReturn(int value);

    unsigned VxWorks64NetFake_HostGetByNameCallCount(void);

    const char* VxWorks64NetFake_LastHostGetByNameName(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64NETFAKE_H */
