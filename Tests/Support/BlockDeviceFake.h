#ifndef BLOCKDEVICEFAKE_H
#define BLOCKDEVICEFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

    /* Faults on the media, each cleared by Create. FlipBit changes the stored
     * byte; a stuck byte reads as @p value whatever is written over it, as a
     * failed program leaves a cell; a failed offset fails every Read whose span
     * covers it, as a bad sector does. */
    void BlockDeviceFake_FlipBit(size_t block, size_t offset, unsigned bit);
    void BlockDeviceFake_StickByte(size_t block, size_t offset, uint8_t value);
    void BlockDeviceFake_FailReadsTouching(size_t block, size_t offset);

    /* Faults on the block set. TruncateTo is what a filesystem that lost a tail
     * reports; RemoveBlock is a file a check deleted; a block that fails every
     * operation is one that reports present but will not open. */
    void BlockDeviceFake_TruncateTo(size_t block, size_t size);
    void BlockDeviceFake_RemoveBlock(size_t block);
    void BlockDeviceFake_FailEveryOperationOn(size_t block);

    /* A block that goes while the store runs: absent once the Nth media
     * operation since Create has completed. */
    void BlockDeviceFake_RemoveBlockAfterOperation(size_t block, size_t operationCount);

    /* What an operation on an absent block meets. Fails is a flash-like device.
     * Created-on-touch is a file device whose open creates the file, so Size
     * answers 0 and the block exists afterwards. Ghost is a file unlinked under
     * an open handle: operations carry on against the old content while Exists
     * says it has gone. */
    enum BlockDeviceFakeAbsentBlockBehaviour
    {
        BLOCKDEVICEFAKE_ABSENT_FAILS,
        BLOCKDEVICEFAKE_ABSENT_IS_CREATED_ON_TOUCH,
        BLOCKDEVICEFAKE_ABSENT_IS_GHOST
    };

    void BlockDeviceFake_SetAbsentBlockBehaviour(enum BlockDeviceFakeAbsentBlockBehaviour behaviour);

    /* Power loss during the next Append: only the first @p bytesThatLand are
     * stored, none for 0. The device still answers true, as the store would
     * have seen had the power held; the test then restarts the store. */
    void BlockDeviceFake_TearNextAppend(size_t bytesThatLand);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BLOCKDEVICEFAKE_H */
