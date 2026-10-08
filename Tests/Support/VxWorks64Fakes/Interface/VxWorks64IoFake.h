#ifndef VXWORKS64IOFAKE_H
#define VXWORKS64IOFAKE_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Drives the stand-in for the I/O calls ioLib.h declares. ioLib.h renames
     * each to VxWorks64IoFake_<Name>, so the C library's own descriptors are
     * never handed to the fake; a test sees only this header.
     *
     * Behind the calls is a small in-memory disk: open with O_CREAT makes a
     * file, read and write move through it from the descriptor's position, and
     * remove deletes it, so a store written through the fake reads back. A
     * scripted answer, when a test gives one, stands in for the disk's. */

    /** Empties the disk and forgets every scripted answer. */
    void VxWorks64IoFake_Reset(void);

    /** The errno that open, read, write and lseek set when they answer ERROR;
     *  EIO until told otherwise. Every failure sets it, so one left by an
     *  earlier call never stands in for it. */
    void VxWorks64IoFake_FailWithErrno(int errnoValue);

    /** Puts an empty file on the disk, as though written by an earlier boot. */
    void VxWorks64IoFake_PutFile(const char* name);

    /** The descriptor open last handed back. */
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

    /** Every byte written since the last reset, to any descriptor, in order. */
    const char* VxWorks64IoFake_Written(void);

    size_t VxWorks64IoFake_WrittenLength(void);

    /** Where the end of the file is: lseek to SEEK_END answers it. */
    void VxWorks64IoFake_SetFileSize(long size);

    /** Make lseek answer ERROR. */
    void VxWorks64IoFake_FailLseeks(void);

    unsigned VxWorks64IoFake_LseekCallCount(void);

    int VxWorks64IoFake_LastLseekFd(void);

    long VxWorks64IoFake_LastLseekOffset(void);

    int VxWorks64IoFake_LastLseekWhence(void);

    /** Make ioctl answer ERROR for this function code, with errno EIO, and OK
     *  for the rest. Every failure sets errno, so one left by an earlier call
     *  never stands in for it. */
    void VxWorks64IoFake_FailIoctl(int function);

    /** As VxWorks64IoFake_FailIoctl, failing with this errno instead. */
    void VxWorks64IoFake_FailIoctlWithErrno(int function, int errnoValue);

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
