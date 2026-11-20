/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSPINORFLASHSFDP_H
#define SOLIDSYSLOGSPINORFLASHSFDP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogSpiNorFlashErrors.h"

/* What a part's SFDP (JESD216) says, read out of bytes the caller fetched.
 * Nothing here touches the bus. */

enum
{
    /* The SFDP header and the first parameter header after it, which is the
     * basic table's. */
    SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_HEADER_BYTES = 16,
    /* As much of the basic table as is read: its first eleven DWORDs, the last
     * of which carries the page size. */
    SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_TABLE_BYTES = 44
};

/* Where the basic table is, and how much of it there is to read. */
struct SolidSyslogSpiNorFlashSfdpTable
{
    uint32_t Address;
    /* Never more than SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_TABLE_BYTES. */
    size_t Bytes;
};

struct SolidSyslogSpiNorFlashGeometry
{
    uint32_t CapacityBytes;
    /* The smallest erase the part has. */
    uint32_t EraseBytes;
};

/* Finds the basic table from the SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_HEADER_BYTES at
 * SFDP address 0. False, with the reason in fault, where they are not the
 * header of an SFDP this reads. */
bool SolidSyslogSpiNorFlashSfdp_Locate(
    const uint8_t* header,
    struct SolidSyslogSpiNorFlashSfdpTable* table,
    enum SolidSyslogSpiNorFlashErrors* fault
);

/* Reads the part's geometry from the bytes of the basic table that Locate
 * found. False, with the reason in fault, for a part outside what the flash
 * drives. */
bool SolidSyslogSpiNorFlashSfdp_ReadGeometry(
    const uint8_t* bytes,
    struct SolidSyslogSpiNorFlashGeometry* geometry,
    enum SolidSyslogSpiNorFlashErrors* fault
);

#endif /* SOLIDSYSLOGSPINORFLASHSFDP_H */
