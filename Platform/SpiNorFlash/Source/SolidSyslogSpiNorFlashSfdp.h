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
    /* One parameter header. Every table an SFDP has is announced by one. */
    SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_PARAMETER_HEADER_BYTES = 8,
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
    /* How many parameter headers the SFDP has after the basic table's. */
    size_t FurtherParameterHeaders;
};

struct SolidSyslogSpiNorFlashGeometry
{
    uint32_t CapacityBytes;
    /* The smallest erase the part has, and the command for it. */
    uint32_t EraseBytes;
    uint8_t EraseCommand;
    /* The most one program command takes. */
    uint32_t PageBytes;
    /* How long to wait for an erase of EraseBytes, and for a program of a
     * page, before giving the part up. */
    uint32_t EraseTimeoutMilliseconds;
    uint32_t ProgramTimeoutMilliseconds;
};

/* Finds the basic table from the SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_HEADER_BYTES at
 * SFDP address 0. False, with the reason in fault, where they are not the
 * header of an SFDP this reads. */
bool SolidSyslogSpiNorFlashSfdp_Locate(
    const uint8_t* header,
    struct SolidSyslogSpiNorFlashSfdpTable* table,
    enum SolidSyslogSpiNorFlashErrors* fault
);

/* The SFDP address of a parameter header after the basic table's. They are
 * numbered from 1, up to the FurtherParameterHeaders Locate gave. */
uint32_t SolidSyslogSpiNorFlashSfdp_ParameterHeaderAddress(size_t number);

/* True where those SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_PARAMETER_HEADER_BYTES are
 * the parameter header of a sector map: the table of a part whose erases are
 * not the same throughout. */
bool SolidSyslogSpiNorFlashSfdp_IsSectorMapHeader(const uint8_t* parameterHeader);

/* Reads the part's geometry from the table->Bytes of the basic table that
 * Locate found. Where the table is too short to give a page size and times,
 * they are the defaults SolidSyslogSpiNorFlash.h states. False, with the
 * reason in fault, for a part outside what the flash drives. */
bool SolidSyslogSpiNorFlashSfdp_ReadGeometry(
    const uint8_t* bytes,
    const struct SolidSyslogSpiNorFlashSfdpTable* table,
    struct SolidSyslogSpiNorFlashGeometry* geometry,
    enum SolidSyslogSpiNorFlashErrors* fault
);

#endif /* SOLIDSYSLOGSPINORFLASHSFDP_H */
