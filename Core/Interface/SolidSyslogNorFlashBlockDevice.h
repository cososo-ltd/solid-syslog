/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A BlockDevice that keeps each block in one erase block of a NOR flash, with
 *  no filesystem between them. It takes a run of the flash's blocks, and
 *  leaves the rest of the part to its owner.
 *
 *  The first bytes of each flash block hold a state word that is advanced only
 *  by clearing bits - in use, then disposed - and the block's data follows it.
 *  A block exists while its word reads exactly as in use. Any other value, a
 *  blank block's included, is a block that is not there. An erase of a
 *  disposed block that power cuts short could, rarely, stop with the in-use
 *  word restored over what the block held. The block then reads as in use: the
 *  store does not jam on it, and at worst reports a few errors for records it
 *  cannot read and delivers again records it had evicted unsent.
 *
 *  Acquire erases the block it takes, whatever the block reads as, and then
 *  marks it in use; Dispose only marks it disposed. No state claims a block is
 *  erased, because an erase cut short by power can leave cells that read as
 *  erased and are not, and an erase repeated is what clears them. So Acquire
 *  takes as long as an erase, and Dispose as long as one short program.
 *
 *  The flash does not say whether a program took, so every Append, WriteAt and
 *  state change is read back, and one that did not take fails the operation
 *  and is reported as SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED.
 *
 *  Size finds the end of a block's data as its last byte that is not 0xFF, so
 *  whatever is appended must end on a byte that is not 0xFF. A BlockStore ends
 *  every record on one. A program that power cut short can leave a record
 *  whose bytes all read as written and are not, which only a SecurityPolicy
 *  that checks the record detects.
 *
 *  Exists and Size read the block's state. Read, Append and WriteAt do not:
 *  they go to the flash block whatever its state, so a Read of a block that
 *  is not there answers with what the cells hold. A BlockStore addresses only
 *  blocks it has acquired or found. A state that cannot be read is a block
 *  that is not there, as the BlockDevice role has no third answer to give. */
#ifndef SOLIDSYSLOGNORFLASHBLOCKDEVICE_H
#define SOLIDSYSLOGNORFLASHBLOCKDEVICE_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogBlockDevice;
struct SolidSyslogNorFlash;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** How many bytes of each flash block the device keeps for the block's state,
     *  ahead of its data. */
    enum
    {
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE = 4
    };

    struct SolidSyslogNorFlashBlockDeviceConfig
    {
        /** The flash the blocks are kept in. It must outlive the device.
         *  SolidSyslogNullNorFlash_Get() asks for no device: Create then returns
         *  the shared NullBlockDevice and reports nothing. */
        struct SolidSyslogNorFlash* Flash;
        /** The first of the flash's blocks the device may use. */
        size_t FirstBlock;
        /** How many of the flash's blocks the device may use, from FirstBlock. It
         *  is what SolidSyslogBlockDevice_GetBlockCount then reports. A BlockStore
         *  asks about every one of them once as it is created, which is a read of
         *  the flash for each. */
        size_t BlockCount;
        /** The size of a block in bytes, as SolidSyslogBlockDevice_GetBlockSize then
         *  reports it. 0 takes all a flash block holds, which is its size less
         *  SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE; more than that is refused.
         *  A smaller value has a BlockStore move on from each block sooner. It is
         *  not a limit the device applies: a BlockStore lets its largest record run
         *  past a block size smaller than one, and the device takes whatever the
         *  flash block has room for. A flash block must hold the store's largest
         *  record, or the Append of one fails. */
        size_t BlockSize;
    };

    /** Keep a BlockDevice's blocks in @p config's run of flash blocks. Nothing is
     *  read or written until the device is used. A NULL @p config or Flash, a run
     *  that is not all within the flash, a flash block too small to hold any data
     *  and a BlockSize more than a flash block holds are each reported as
     *  SOLIDSYSLOG_CAT_BAD_CONFIG, and Create returns the shared NullBlockDevice. So
     *  does an exhausted pool, reported as SOLIDSYSLOG_CAT_POOL_EXHAUSTED. */
    struct SolidSyslogBlockDevice* SolidSyslogNorFlashBlockDevice_Create(
        const struct SolidSyslogNorFlashBlockDeviceConfig* config
    );

    /** Release the pool slot. The flash is not touched: the blocks stay as they
     *  are for the next device created over them. */
    void SolidSyslogNorFlashBlockDevice_Destroy(struct SolidSyslogBlockDevice * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNORFLASHBLOCKDEVICE_H */
