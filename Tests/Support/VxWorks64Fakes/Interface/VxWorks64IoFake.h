#ifndef VXWORKS64IOFAKE_H
#define VXWORKS64IOFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Drives the stand-in for the I/O calls ioLib.h declares. ioLib.h renames
     * each to VxWorks64IoFake_<Name>, so the C library's own descriptors are
     * never handed to the fake; a test sees only this header. */

    void VxWorks64IoFake_Reset(void);

    unsigned VxWorks64IoFake_CloseCallCount(void);

    int VxWorks64IoFake_LastClosedFd(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64IOFAKE_H */
