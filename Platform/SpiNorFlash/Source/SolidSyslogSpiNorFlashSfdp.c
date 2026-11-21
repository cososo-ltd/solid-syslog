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
    /* How many parameter headers follow the first. */
    HEADER_FURTHER_PARAMETER_HEADERS = 6,
    SFDP_HEADER_BYTES = 8,
    /* The first parameter header follows the eight bytes of the SFDP header. */
    TABLE_ID_LOW = 8,
    TABLE_MAJOR_REVISION = 10,
    TABLE_LENGTH_DWORDS = 11,
    TABLE_POINTER = 12,
    TABLE_ID_HIGH = 15,

    /* Within any one parameter header. */
    PARAMETER_ID_LOW = 0,
    PARAMETER_ID_HIGH = 7,

    MAJOR_REVISION_READ = 1,
    BASIC_TABLE_ID_LOW = 0x00,
    BASIC_TABLE_ID_HIGH = 0xFF,
    SECTOR_MAP_ID_LOW = 0x81,
    SECTOR_MAP_ID_HIGH = 0xFF,
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
    LARGEST_ERASE_EXPONENT = 24,

    /* Both arrived with JESD216A. Each holds, in its lowest four bits, what a
     * typical time is multiplied by to give the longest. */
    DWORD_ERASE_TIMES = 10,
    DWORD_PAGE_AND_PROGRAM = 11,
    LONGEST_TIME_MASK = 0xF,
    /* A typical time is a count less one, in the low five bits of its field,
     * of a unit the bits above choose. */
    TIME_COUNT_MASK = 0x1F,

    /* Seven bits for each erase type, the first at bit 4. */
    ERASE_TIME_SHIFT = 4,
    ERASE_TIME_BITS = 7,
    ERASE_TIME_UNITS_SHIFT = 5,
    ERASE_TIME_UNITS_MASK = 0x3,
    ERASE_TIME_UNITS_COUNT = 4,

    PAGE_SIZE_SHIFT = 4,
    PAGE_SIZE_MASK = 0xF,
    PROGRAM_TIME_SHIFT = 8,
    PROGRAM_TIME_UNITS_ARE_LONG = 0x2000,
    PROGRAM_TIME_SHORT_UNITS = 8,
    PROGRAM_TIME_LONG_UNITS = 64,
    MICROSECONDS_PER_MILLISECOND = 1000,

    /* What a table too short to say is taken to mean. */
    DEFAULT_PAGE_BYTES = 256,
    DEFAULT_ERASE_TIMEOUT_MILLISECONDS = 3000,
    DEFAULT_PROGRAM_TIMEOUT_MILLISECONDS = 20,

    /* Added to a time the table gives. The count of milliseconds may be about
     * to move on as a wait begins, which would end the wait a millisecond
     * early. */
    TICK_ALLOWANCE_MILLISECONDS = 1
};

static const uint32_t DENSITY_IS_AN_EXPONENT = 0x80000000U;

static inline bool Sfdp_HasSignature(const uint8_t* header);
static inline bool Sfdp_BeginsWithABasicTable(const uint8_t* header);
static inline size_t Sfdp_BytesToRead(uint8_t tableDwords);
static inline uint32_t Sfdp_Dword(const uint8_t* bytes, size_t number);
static inline bool Sfdp_TakesThreeByteAddresses(const uint8_t* bytes);
static inline bool Sfdp_DensityIsWithinReach(uint32_t density);
static inline uint32_t Sfdp_CapacityBytes(uint32_t density);
static inline const uint8_t* Sfdp_EraseTypes(const uint8_t* bytes);
static size_t Sfdp_SmallestEraseType(const uint8_t* types);
static inline bool Sfdp_EraseFitsIn(uint8_t exponent, uint32_t capacityBytes);
static inline bool Sfdp_HasTimesAndPageSize(const struct SolidSyslogSpiNorFlashSfdpTable* table);
static inline uint32_t Sfdp_PageBytes(const uint8_t* bytes);
static uint32_t Sfdp_EraseTimeoutMilliseconds(const uint8_t* bytes, size_t eraseType);
static inline uint32_t Sfdp_TimeCount(uint32_t field);
static inline uint32_t Sfdp_LongestTimeMultiplier(uint32_t dword);
static uint32_t Sfdp_ProgramTimeoutMilliseconds(const uint8_t* bytes);

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
        table->FurtherParameterHeaders = (size_t) header[HEADER_FURTHER_PARAMETER_HEADERS];
        located = true;
    }

    return located;
}

static inline bool Sfdp_HasSignature(const uint8_t* header)
{
    /* "SFDP". */
    static const uint8_t signature[SIGNATURE_LENGTH] = {0x53, 0x46, 0x44, 0x50};
    bool matches = true;

    for (size_t i = 0U; i < (size_t) SIGNATURE_LENGTH; i++)
    {
        if (header[i] != signature[i])
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

/* At most 255 headers of eight bytes, so nothing overflows. */
uint32_t SolidSyslogSpiNorFlashSfdp_ParameterHeaderAddress(size_t number)
{
    return (uint32_t) SFDP_HEADER_BYTES +
           ((uint32_t) number * (uint32_t) SOLIDSYSLOG_SPI_NOR_FLASH_SFDP_PARAMETER_HEADER_BYTES);
}

bool SolidSyslogSpiNorFlashSfdp_IsSectorMapHeader(const uint8_t* parameterHeader)
{
    return (parameterHeader[PARAMETER_ID_LOW] == (uint8_t) SECTOR_MAP_ID_LOW) &&
           (parameterHeader[PARAMETER_ID_HIGH] == (uint8_t) SECTOR_MAP_ID_HIGH);
}

bool SolidSyslogSpiNorFlashSfdp_ReadGeometry(
    const uint8_t* bytes,
    const struct SolidSyslogSpiNorFlashSfdpTable* table,
    struct SolidSyslogSpiNorFlashGeometry* geometry,
    enum SolidSyslogSpiNorFlashErrors* fault
)
{
    const uint8_t* eraseTypes = Sfdp_EraseTypes(bytes);
    size_t eraseType = Sfdp_SmallestEraseType(eraseTypes);
    uint8_t eraseExponent = eraseTypes[eraseType * (size_t) ERASE_TYPE_BYTES];
    uint32_t density = Sfdp_Dword(bytes, DWORD_DENSITY);
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
        geometry->EraseCommand = eraseTypes[(eraseType * (size_t) ERASE_TYPE_BYTES) + 1U];
        geometry->PageBytes = (uint32_t) DEFAULT_PAGE_BYTES;
        geometry->EraseTimeoutMilliseconds = (uint32_t) DEFAULT_ERASE_TIMEOUT_MILLISECONDS;
        geometry->ProgramTimeoutMilliseconds = (uint32_t) DEFAULT_PROGRAM_TIMEOUT_MILLISECONDS;

        if (Sfdp_HasTimesAndPageSize(table))
        {
            geometry->PageBytes = Sfdp_PageBytes(bytes);
            geometry->EraseTimeoutMilliseconds = Sfdp_EraseTimeoutMilliseconds(bytes, eraseType);
            geometry->ProgramTimeoutMilliseconds = Sfdp_ProgramTimeoutMilliseconds(bytes);
        }

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

static inline const uint8_t* Sfdp_EraseTypes(const uint8_t* bytes)
{
    return &bytes[((size_t) DWORD_ERASE_TYPES - 1U) * (size_t) DWORD_BYTES];
}

/* Which of the four types is the smallest the part has. Where it has none,
 * the first, whose size is then NO_ERASE_TYPE. */
static size_t Sfdp_SmallestEraseType(const uint8_t* types)
{
    size_t smallest = 0U;

    for (size_t i = 1U; i < (size_t) ERASE_TYPE_COUNT; i++)
    {
        uint8_t exponent = types[i * (size_t) ERASE_TYPE_BYTES];
        uint8_t smallestExponent = types[smallest * (size_t) ERASE_TYPE_BYTES];

        if ((exponent != (uint8_t) NO_ERASE_TYPE) &&
            ((smallestExponent == (uint8_t) NO_ERASE_TYPE) || (exponent < smallestExponent)))
        {
            smallest = i;
        }
    }

    return smallest;
}

static inline bool Sfdp_EraseFitsIn(uint8_t exponent, uint32_t capacityBytes)
{
    return (exponent != (uint8_t) NO_ERASE_TYPE) && (exponent <= (uint8_t) LARGEST_ERASE_EXPONENT) &&
           (((uint32_t) 1U << exponent) <= capacityBytes);
}

static inline bool Sfdp_HasTimesAndPageSize(const struct SolidSyslogSpiNorFlashSfdpTable* table)
{
    return table->Bytes >= ((size_t) DWORD_PAGE_AND_PROGRAM * (size_t) DWORD_BYTES);
}

static inline uint32_t Sfdp_PageBytes(const uint8_t* bytes)
{
    uint32_t exponent =
        (Sfdp_Dword(bytes, DWORD_PAGE_AND_PROGRAM) >> (uint32_t) PAGE_SIZE_SHIFT) & (uint32_t) PAGE_SIZE_MASK;

    return (uint32_t) 1U << exponent;
}

/* The longest the erase of that type takes, by the table. At most 32 counts
 * of a second, 32 times over, so nothing overflows. */
static uint32_t Sfdp_EraseTimeoutMilliseconds(const uint8_t* bytes, size_t eraseType)
{
    /* Milliseconds, by the two bits above an erase time's count. */
    static const uint32_t timeUnits[ERASE_TIME_UNITS_COUNT] = {1U, 16U, 128U, 1000U};
    uint32_t dword = Sfdp_Dword(bytes, DWORD_ERASE_TIMES);
    uint32_t field = dword >> ((uint32_t) ERASE_TIME_SHIFT + ((uint32_t) eraseType * (uint32_t) ERASE_TIME_BITS));
    uint32_t units = timeUnits[(field >> (uint32_t) ERASE_TIME_UNITS_SHIFT) & (uint32_t) ERASE_TIME_UNITS_MASK];
    uint32_t typical = Sfdp_TimeCount(field) * units;

    return (typical * Sfdp_LongestTimeMultiplier(dword)) + (uint32_t) TICK_ALLOWANCE_MILLISECONDS;
}

static inline uint32_t Sfdp_TimeCount(uint32_t field)
{
    return (field & (uint32_t) TIME_COUNT_MASK) + 1U;
}

static inline uint32_t Sfdp_LongestTimeMultiplier(uint32_t dword)
{
    return 2U * ((dword & (uint32_t) LONGEST_TIME_MASK) + 1U);
}

/* The longest a page program takes, by the table, which gives it in
 * microseconds. Rounded up to a whole millisecond. */
static uint32_t Sfdp_ProgramTimeoutMilliseconds(const uint8_t* bytes)
{
    uint32_t dword = Sfdp_Dword(bytes, DWORD_PAGE_AND_PROGRAM);
    uint32_t units = (uint32_t) PROGRAM_TIME_SHORT_UNITS;

    if ((dword & (uint32_t) PROGRAM_TIME_UNITS_ARE_LONG) != 0U)
    {
        units = (uint32_t) PROGRAM_TIME_LONG_UNITS;
    }

    uint32_t typical = Sfdp_TimeCount(dword >> (uint32_t) PROGRAM_TIME_SHIFT) * units;
    uint32_t longest = typical * Sfdp_LongestTimeMultiplier(dword);
    uint32_t roundedUp =
        (longest + (uint32_t) MICROSECONDS_PER_MILLISECOND - 1U) / (uint32_t) MICROSECONDS_PER_MILLISECOND;

    return roundedUp + (uint32_t) TICK_ALLOWANCE_MILLISECONDS;
}
