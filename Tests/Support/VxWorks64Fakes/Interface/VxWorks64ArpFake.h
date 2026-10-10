#ifndef VXWORKS64ARPFAKE_H
#define VXWORKS64ARPFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void VxWorks64ArpFake_Reset(void);

    /** Make arpResolve answer ERROR with errno set to the value given. */
    void VxWorks64ArpFake_FailWithErrno(int errnoValue);

    unsigned VxWorks64ArpFake_ArpResolveCallCount(void);

    /** The address arpResolve was last asked for, copied when it was called. */
    const char* VxWorks64ArpFake_LastTarget(void);

    int VxWorks64ArpFake_LastNumTries(void);

    int VxWorks64ArpFake_LastNumTicks(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64ARPFAKE_H */
