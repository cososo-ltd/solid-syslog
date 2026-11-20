/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSpiNorFlashSfdp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogSpiNorFlashErrors.h"

/* Field positions are those of JESD216, whose DWORDs are numbered from 1 and
 * stored least significant byte first. */
enum
{
    SIGNATURE_LENGTH = 4,
    HEADER_MAJOR_REVISION = 5,
    /* The first parameter header follows the eight bytes of the SFDP header. */
    TABLE_ID_LOW = 8,
    TABLE_MAJOR_REVISION = 10,
    TABLE_LENGTH_DWORDS = 11,
    TABLE_POINTER = 12,
    TABLE_ID_HIGH = 15,

    MAJOR_REVISION_READ = 1,
    BASIC_TABLE_ID_LOW = 0x00,
    BASIC_TABLE_ID_HIGH = 0xFF,
    DWORD_BYTES = 4,
    /* The basic table as JESD216 first defined it. */
    SHORTEST_BASIC_TABLE_DWORDS = 9,

    DWORD_ADDRESS_BYTES = 1,
    ADDRESS_BYTES_SHIFT = 17,
    ADDRESS_BYTES_MASK = 0x3,
    /* 0 is three bytes only and 1 is three or four. */
    ADDRESS_BYTES_FOUR_ONLY = 2,

    DWORD_DENSITY = 2,
    /* 16 MiB in bits, as a power of two. */
    LARGEST_DENSITY_EXPONENT = 27,
    BITS_PER_BYTE = 8,

    /* Four erase types, each a size as a power of two and then a command. A
     * size of zero is a type the part does not have. */
    DWORD_ERASE_TYPES = 8,
    ERASE_TYPE_COUNT = 4,
    ERASE_TYPE_BYTES = 2,
    NO_ERASE_TYPE = 0,
    /* 16 MiB, the largest part, as a power of two. */
    LARGEST_ERASE_EXPONENT = 24
};

static const uint8_t Sfdp_Signature[SIGNATURE_LENGTH] = {0x53, 0x46, 0x44, 0x50};

static const uint32_t DENSITY_IS_AN_EXPONENT = 0x80000000U;

static inline bool Sfdp_HasSignature(const uint8_t* header);
static inline bool Sfdp_BeginsWithABasicTable(const uint8_t* header);
static inline size_t Sfdp_BytesToRead(uint8_t tableDwords);
static inline uint32_t Sfdp_Dword(const uint8_t* bytes, size_t number);
static inline bool Sfdp_TakesThreeByteAddresses(const uint8_t* bytes);
static inline bool Sfdp_DensityIsWithinReach(uint32_t density);
static inline uint32_t Sfdp_CapacityBytes(uint32_t density);
static uint8_t Sfdp_SmallestEraseExponent(const uint8_t* bytes);
static inline bool Sfdp_EraseFitsIn(uint8_t exponent, uint32_t capacityBytes);

bool SolidSyslogSpiNorFlashSfdp_Locate(
    const uint8_t* header,
    struct SolidSyslogSpiNorFlashSfdpTable* table,
    enum SolidSyslogSpiNorFlashErrors* fault
)
{
    bool located = false;

    if (!Sfdp_HasSignature(header))
    {
        *fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP;
    }
    else if (!Sfdp_BeginsWithABasicTable(header))
    {
        *fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD;
    }
    else
    {
        table->Address = (uint32_t) header[TABLE_POINTER] | ((uint32_t) header[TABLE_POINTER + 1] << 8U) |
                         ((uint32_t) header[TABLE_POINTER + 2] << 16U);
        table->Bytes = Sfdp_BytesToRead(header[TABLE_LENGTH_DWORDS]);
        located = true;
    }

    return located;
}

static inline bool Sfdp_HasSignature(const uint8_t* header)
{
    bool matches = true;

    for (size_t i = 0U; i < (size_t) SIGNATURE_LENGTH; i++)
    {
        if (header[i] != Sfdp_Signature[i])
        {
            matches = false;
        }
    }

    return matches;
}

static inline bool Sfdp_BeginsWithABasicTable(const uint8_t* header)
{
    return (header[HEADER_MAJOR_REVISION] == (uint8_t) MAJOR_REVISION_READ) &&
           (header[TABLE_ID_LOW] == (uint8_t) BASIC_TABLE_ID_LOW) &&
           (header[TABLE_ID_HIGH] == (uint8_t) BASIC_TABLE_ID_HIGH) &&
           (header[TABLE_MAJOR_REVISION] == (uint8_t) MAJOR_REVISION_READ) &&
           (header[TABLE_LENGTH_DWORDS] >= (uint8_t) SHORTEST_BASIC_TABLE_DWORDS);
}

static inline size_t Sfdp_BytesToRead(uint8_t tableDwords)
{
    size_t bytes = (size_t) tableDwords * (size_t) DWORD_BYTES;

    if (bytes > (size_t) SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_TABLE_BYTES)
    {
        bytes = (size_t) SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_TABLE_BYTES;
    }

    return bytes;
}

bool SolidSyslogSpiNorFlashSfdp_ReadGeometry(
    const uint8_t* bytes,
    struct SolidSyslogSpiNorFlashGeometry* geometry,
    enum SolidSyslogSpiNorFlashErrors* fault
)
{
    uint32_t density = Sfdp_Dword(bytes, DWORD_DENSITY);
    uint8_t eraseExponent = Sfdp_SmallestEraseExponent(bytes);
    bool read = false;

    if (!Sfdp_TakesThreeByteAddresses(bytes))
    {
        *fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_FOUR_BYTE_ADDRESSES_ONLY;
    }
    else if (!Sfdp_DensityIsWithinReach(density))
    {
        *fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_TOO_LARGE;
    }
    else if (!Sfdp_EraseFitsIn(eraseExponent, Sfdp_CapacityBytes(density)))
    {
        *fault = SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD;
    }
    else
    {
        geometry->CapacityBytes = Sfdp_CapacityBytes(density);
        geometry->EraseBytes = (uint32_t) 1U << eraseExponent;
        read = true;
    }

    return read;
}

static inline uint32_t Sfdp_Dword(const uint8_t* bytes, size_t number)
{
    const uint8_t* at = &bytes[(number - 1U) * (size_t) DWORD_BYTES];

    return (uint32_t) at[0] | ((uint32_t) at[1] << 8U) | ((uint32_t) at[2] << 16U) | ((uint32_t) at[3] << 24U);
}

static inline bool Sfdp_TakesThreeByteAddresses(const uint8_t* bytes)
{
    uint32_t addressBytes =
        (Sfdp_Dword(bytes, DWORD_ADDRESS_BYTES) >> (uint32_t) ADDRESS_BYTES_SHIFT) & (uint32_t) ADDRESS_BYTES_MASK;

    return addressBytes < (uint32_t) ADDRESS_BYTES_FOUR_ONLY;
}

/* The density is in bits: a power of two where the top bit is set, and
 * otherwise one less than the number of them. */
static inline bool Sfdp_DensityIsWithinReach(uint32_t density)
{
    bool withinReach = false;

    if ((density & DENSITY_IS_AN_EXPONENT) != 0U)
    {
        withinReach = (density & ~DENSITY_IS_AN_EXPONENT) <= (uint32_t) LARGEST_DENSITY_EXPONENT;
    }
    else
    {
        withinReach = density < ((uint32_t) 1U << (uint32_t) LARGEST_DENSITY_EXPONENT);
    }

    return withinReach;
}

/* For a density Sfdp_DensityIsWithinReach has passed, so nothing overflows. */
static inline uint32_t Sfdp_CapacityBytes(uint32_t density)
{
    uint32_t bits = density + 1U;

    if ((density & DENSITY_IS_AN_EXPONENT) != 0U)
    {
        bits = (uint32_t) 1U << (density & ~DENSITY_IS_AN_EXPONENT);
    }

    return bits / (uint32_t) BITS_PER_BYTE;
}

/* NO_ERASE_TYPE where the table lists none. */
static uint8_t Sfdp_SmallestEraseExponent(const uint8_t* bytes)
{
    const uint8_t* types = &bytes[((size_t) DWORD_ERASE_TYPES - 1U) * (size_t) DWORD_BYTES];
    uint8_t smallest = (uint8_t) NO_ERASE_TYPE;

    for (size_t i = 0U; i < (size_t) ERASE_TYPE_COUNT; i++)
    {
        uint8_t exponent = types[i * (size_t) ERASE_TYPE_BYTES];

        if ((exponent != (uint8_t) NO_ERASE_TYPE) && ((smallest == (uint8_t) NO_ERASE_TYPE) || (exponent < smallest)))
        {
            smallest = exponent;
        }
    }

    return smallest;
}

static inline bool Sfdp_EraseFitsIn(uint8_t exponent, uint32_t capacityBytes)
{
    return (exponent != (uint8_t) NO_ERASE_TYPE) && (exponent <= (uint8_t) LARGEST_ERASE_EXPONENT) &&
           (((uint32_t) 1U << exponent) <= capacityBytes);
}
