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
     *  header, every parameter header, and the first nine DWORDs of one table,
     *  the basic parameter table, for the part's capacity, its address width,
     *  and its smallest erase with that erase's command.
     *
     *  A block of the NorFlash is one erase of the smallest size the table
     *  lists, and the blocks cover the whole part.
     *
     *  Nothing else is taken from the table. On any part an erase is allowed
     *  3000 ms and a page program 20 ms, each well beyond what a part in good
     *  order takes, and a program is sent no more than 256 bytes at a time,
     *  split where a page of that size ends. So the part's page must be 256
     *  bytes or a multiple of it.
     *
     *  Refused, each reported as SOLIDSYSLOG_CAT_BAD_CONFIG:
     *   - a NULL @p config or Bus, or a Bus with a member not filled;
     *   - a part with no SFDP table, or one whose table is not understood;
     *   - a part of more than 16 MiB, the most three address bytes reach;
     *   - a part that takes only four-byte addresses;
     *   - a part whose SFDP has a parameter header for a sector map. Such a
     *     part does not erase the same way throughout, and the map that says
     *     where each erase applies is not read.
     *
     *  A part that does not answer, one still busy after the wait, and a bus
     *  call that fails are each reported as SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED.
     *  A line held low, as a part with no power may leave it, reads as a part
     *  that is ready, so it is refused for the SFDP it does not have.
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
     *  operation, at once, until it is ready again.
     *
     *  Three things the flash does not check:
     *   - The span of a Read, a Program or an Erase. The NorFlash role makes
     *     it the caller's to keep within a block, and an address past the end
     *     of a part wraps to its start.
     *   - Write protection, which is the part's owner's business. The flash
     *     neither looks for it nor changes it: it never writes a status
     *     register. A part takes a program or an erase of a block it protects
     *     and does nothing, without saying so, so that Program or Erase
     *     answers true. The SolidSyslogNorFlashBlockDevice above reads back
     *     what it programs, and reports the difference as
     *     SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED.
     *   - A line held low once the flash is created. A Read then answers true
     *     with bytes of zero; a Program or an Erase fails, because writing
     *     does not show as enabled. */
    struct SolidSyslogNorFlash* SolidSyslogSpiNorFlash_Create(const struct SolidSyslogSpiNorFlashConfig* config);

    /** Release the pool slot. The part is not touched. */
    void SolidSyslogSpiNorFlash_Destroy(struct SolidSyslogNorFlash * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSPINORFLASH_H */
