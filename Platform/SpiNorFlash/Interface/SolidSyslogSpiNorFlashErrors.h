/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the SpiNorFlash. */
#ifndef SOLIDSYSLOGSPINORFLASHERRORS_H
#define SOLIDSYSLOGSPINORFLASHERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogSpiNorFlashErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogSpiNorFlashErrors
    {
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NULL_BUS,
        /** A member of the bus table is not filled. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_INCOMPLETE,
        /** The part did not answer a request for its SFDP table with one. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP,
        /** The part has an SFDP table, and it is not one this reads: its
         *  revision is not 1.x, its first table is not the basic one or is
         *  shorter than JESD216 defines, or it lists no erase the part's
         *  capacity is a whole number of. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD,
        /** The part is larger than 16 MiB. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_TOO_LARGE,
        /** The part takes only four-byte addresses. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_FOUR_BYTE_ADDRESSES_ONLY,
        /** The part's SFDP has a sector map, the table of a part that does
         *  not erase the same way throughout. The map is not read, so where
         *  the smallest erase applies is not known. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_HAS_SECTOR_MAP,
        /** A protection bit of the part's first status register is set. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_PROTECTED,
        /** Nothing drives the bus: no part, or no path to it. Raised under
         *  SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED, as are the three after it. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER,
        /** A call of the bus answered false. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED,
        /** The part was still busy when the time allowed had passed. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT,
        /** The part would not enable writing, so the command that needed it
         *  was not sent. A Program over several pages sends one for each, so
         *  the pages before the one refused may already be programmed. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_WRITE_REFUSED,
        /** A read, a program or an erase was asked for outside a block of the
         *  part. Raised under SOLIDSYSLOG_CAT_BAD_ARGUMENT, and nothing was
         *  sent to the part. */
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_OUT_OF_RANGE,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The detail's name, read beside the category's: NO_SFDP for
     *  SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP. NULL for a code outside the enum. */
    const char* SolidSyslogSpiNorFlashErrors_Name(int32_t detail);

    /** The identity for events raised by a SpiNorFlash. A handler matches by
     *  address (event->Source == &SolidSyslogSpiNorFlashErrorSource), then reads
     *  event->Detail as an enum SolidSyslogSpiNorFlashErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogSpiNorFlashErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSPINORFLASHERRORS_H */
