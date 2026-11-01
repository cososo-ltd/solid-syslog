#ifndef BLOCKDEVICEFAKE_H
#define BLOCKDEVICEFAKE_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogBlockDevice;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* An in-memory block device holding the whole block space, for driving the
     * store through media states no file fake can express. One instance; Create
     * resets it. */
    struct SolidSyslogBlockDevice* BlockDeviceFake_Create(size_t blockSize);

    /* Media operations since Create: every vtable call but GetBlockSize, which
     * reads a constant. Bounds the work a single store call may do. */
    size_t BlockDeviceFake_OperationCount(void);

    /* One write the device accepted. Appended says whether it was an Append
     * (Offset is then where the block ended) rather than a WriteAt. */
    struct BlockDeviceFakeWrite
    {
        size_t Block;
        size_t Offset;
        size_t Count;
        bool Appended;
    };

    /* Writes the device accepted since Create, in order. A refused write is
     * not logged. Entries past what is kept read back zeroed. */
    size_t BlockDeviceFake_WriteCount(void);
    struct BlockDeviceFakeWrite BlockDeviceFake_Write(size_t index);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BLOCKDEVICEFAKE_H */
