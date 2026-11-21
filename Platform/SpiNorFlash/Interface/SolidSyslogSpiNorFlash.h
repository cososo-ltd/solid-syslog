/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A NorFlash over an SPI NOR flash part, for a SolidSyslogNorFlashBlockDevice.
 *  It learns the part from the part's own SFDP table (JESD216), so it is not
 *  written for one maker's parts, and it refuses at Create a part it cannot
 *  drive. */
#ifndef SOLIDSYSLOGSPINORFLASH_H
#define SOLIDSYSLOGSPINORFLASH_H

#include "SolidSyslogExternC.h"

struct SolidSyslogNorFlash;
struct SolidSyslogSpiNorFlashBus;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSpiNorFlashConfig
    {
        /** The bus the part is on, ready for use. It must outlive the flash. */
        struct SolidSyslogSpiNorFlashBus* Bus;
    };

    /** Start the part on @p config's bus and learn it from its SFDP table.
     *
     *  Create talks to the part. It first waits for the part to be ready, for up
     *  to 3000 ms: a part that was erasing when the processor was reset carries
     *  on, and answers nothing else until it finishes. It then reads the SFDP
     *  header and every parameter header, and one table, the basic parameter
     *  table, for the part's capacity, its address width, its smallest erase
     *  and that erase's command, its page size, and the longest an erase and a
     *  page program take.
     *
     *  A block of the NorFlash is one erase of the smallest size the table
     *  lists, and the blocks cover the whole part.
     *
     *  An erase and a page program are each allowed the longest the table says
     *  one takes, rounded up to a whole millisecond, and one millisecond more.
     *  A table older than JESD216A carries no page size and no times. For such
     *  a part the page is taken as 256 bytes, an erase is allowed 3000 ms and a
     *  page program 20 ms.
     *
     *  Refused, each reported as SOLIDSYSLOG_CAT_BAD_CONFIG:
     *   - a NULL @p config or Bus, or a Bus with a member not filled;
     *   - a part with no SFDP table, or one whose table is not understood;
     *   - a part of more than 16 MiB, the most three address bytes reach;
     *   - a part that takes only four-byte addresses;
     *   - a part whose SFDP has a parameter header for a sector map. Such a
     *     part does not erase the same way throughout, and the map that says
     *     where each erase applies is not read;
     *   - a part with any of bits 2 to 6 of its first status register set,
     *     which protect some or all of it from being written. The flash never
     *     writes a status register, so it never unlocks a part.
     *
     *  Protection a part holds anywhere else is not seen: a bit of another
     *  status register, or a lock on each block. Such a part is created, and
     *  a program or an erase it ignores answers true. The
     *  SolidSyslogNorFlashBlockDevice above reads back what it programs, and
     *  reports the difference as
     *  SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED.
     *
     *  A part that does not answer, one still busy after the wait, and a bus
     *  call that fails are each reported as SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED.
     *
     *  Every one of those returns SolidSyslogNullNorFlash, as an exhausted pool
     *  does, reported as SOLIDSYSLOG_CAT_POOL_EXHAUSTED. event->Detail is an
     *  enum SolidSyslogSpiNorFlashErrors saying which.
     *
     *  Once created, a Read, a Program or an Erase that fails answers false and
     *  is reported as SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED: an erase or a page
     *  program the part has not finished in the time allowed, a part that will
     *  not enable writing or has stopped answering, and a bus call that fails.
     *  A part left busy by an operation that outlasted its time fails every
     *  operation, at once, until it is ready again. A span that is not within
     *  one block is reported as SOLIDSYSLOG_CAT_BAD_ARGUMENT, and nothing is
     *  sent for it. */
    struct SolidSyslogNorFlash* SolidSyslogSpiNorFlash_Create(const struct SolidSyslogSpiNorFlashConfig* config);

    /** Release the pool slot. The part is not touched. */
    void SolidSyslogSpiNorFlash_Destroy(struct SolidSyslogNorFlash * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSPINORFLASH_H */
