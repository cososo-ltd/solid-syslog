#ifndef VXWORKS64IOFAKE_H
#define VXWORKS64IOFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Drives the stand-in for the I/O calls ioLib.h declares. ioLib.h renames
     * each to VxWorks64IoFake_<Name>, so the C library's own descriptors are
     * never handed to the fake; a test sees only this header. */

    void VxWorks64IoFake_Reset(void);

    /** The descriptor open hands back when it succeeds. */
    int VxWorks64IoFake_Fd(void);

    /** Make open answer ERROR, as it does for a path it cannot open. */
    void VxWorks64IoFake_FailOpens(void);

    unsigned VxWorks64IoFake_OpenCallCount(void);

    const char* VxWorks64IoFake_LastOpenName(void);

    int VxWorks64IoFake_LastOpenFlags(void);

    int VxWorks64IoFake_LastOpenMode(void);

    /** Make read deliver these bytes, as many as fit the count it is asked
     *  for, and answer how many it delivered. Until told otherwise it
     *  delivers nothing, as at the end of a file. */
    void VxWorks64IoFake_ReadDelivers(const char* data, int length);

    /** Make read answer ERROR. */
    void VxWorks64IoFake_FailReads(void);

    unsigned VxWorks64IoFake_ReadCallCount(void);

    int VxWorks64IoFake_LastReadFd(void);

    int VxWorks64IoFake_LastReadMaxBytes(void);

    /** Make write answer this many bytes written, or ERROR given -1. Until
     *  told otherwise it writes all it is given. */
    void VxWorks64IoFake_WriteAccepts(int written);

    unsigned VxWorks64IoFake_WriteCallCount(void);

    int VxWorks64IoFake_LastWriteFd(void);

    const char* VxWorks64IoFake_LastWriteBuffer(void);

    int VxWorks64IoFake_LastWriteNBytes(void);

    /** Where the end of the file is: lseek to SEEK_END answers it. */
    void VxWorks64IoFake_SetFileSize(long size);

    /** Make lseek answer ERROR. */
    void VxWorks64IoFake_FailLseeks(void);

    unsigned VxWorks64IoFake_LseekCallCount(void);

    int VxWorks64IoFake_LastLseekFd(void);

    long VxWorks64IoFake_LastLseekOffset(void);

    int VxWorks64IoFake_LastLseekWhence(void);

    /** Make ioctl answer ERROR for this function code, and OK for the rest. */
    void VxWorks64IoFake_FailIoctl(int function);

    unsigned VxWorks64IoFake_IoctlCallCount(void);

    /** The descriptor, function and argument of the call-th ioctl since the
     *  last reset, counting from 0; -1 for a call not made. */
    int VxWorks64IoFake_IoctlFd(unsigned call);

    int VxWorks64IoFake_IoctlFunction(unsigned call);

    int VxWorks64IoFake_IoctlArg(unsigned call);

    /** Make remove answer ERROR. */
    void VxWorks64IoFake_FailRemoves(void);

    unsigned VxWorks64IoFake_RemoveCallCount(void);

    const char* VxWorks64IoFake_LastRemoveName(void);

    unsigned VxWorks64IoFake_CloseCallCount(void);

    int VxWorks64IoFake_LastClosedFd(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64IOFAKE_H */
