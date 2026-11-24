/* A W25Q128JV in software - see SimulatedSpiNorFlash.h. */

#include "SimulatedSpiNorFlash.h"

#include "SolidSyslogSpiNorFlashBus.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void SimulatedSpiNorFlash_Select(struct SolidSyslogSpiNorFlashBus* base);
static void SimulatedSpiNorFlash_Deselect(struct SolidSyslogSpiNorFlashBus* base);
static bool SimulatedSpiNorFlash_Write(struct SolidSyslogSpiNorFlashBus* base, const void* buffer, size_t count);
static bool SimulatedSpiNorFlash_Read(struct SolidSyslogSpiNorFlashBus* base, void* buffer, size_t count);
static uint32_t SimulatedSpiNorFlash_Milliseconds(struct SolidSyslogSpiNorFlashBus* base);

static struct SolidSyslogSpiNorFlashBus bus = {
    SimulatedSpiNorFlash_Select,
    SimulatedSpiNorFlash_Deselect,
    SimulatedSpiNorFlash_Write,
    SimulatedSpiNorFlash_Read,
    SimulatedSpiNorFlash_Milliseconds,
    NULL,
};

enum
{
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    READ_SFDP = 0x5A,

    /* What a line with nothing driving it reads as. */
    IDLE_LINE = 0xFF,

    /* A W25Q128JV's. */
    PART_BYTES = 16 * 1024 * 1024,

    COMMAND_BYTES = 1,
    ADDRESS_BYTES = 3,
    COMMAND_AND_ADDRESS_BYTES = COMMAND_BYTES + ADDRESS_BYTES,
    /* The command, its address, and one byte the part ignores. */
    SFDP_REQUEST_BYTES = COMMAND_AND_ADDRESS_BYTES + 1,
    /* The part answers the same bytes again every this many. */
    SFDP_BYTES = 256,
    SFDP_HEADER_BYTES = 16,
    SFDP_TABLE_ADDRESS = 0x80,
    SFDP_TABLE_BYTES = 64
};

/* What a W25Q128JV answers, as it was read from one: SFDP 1.5, one parameter
 * header, and a basic table of sixteen DWORDs. Sixteen bytes to a row. */
// clang-format off
static const uint8_t SFDP_HEADER[SFDP_HEADER_BYTES] = {
    0x53, 0x46, 0x44, 0x50, 0x05, 0x01, 0x00, 0xFF, 0x00, 0x05, 0x01, 0x10, 0x80, 0x00, 0x00, 0xFF
};

static const uint8_t SFDP_TABLE[SFDP_TABLE_BYTES] = {
    0xE5, 0x20, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x44, 0xEB, 0x08, 0x6B, 0x08, 0x3B, 0x42, 0xBB,
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x40, 0xEB, 0x0C, 0x20, 0x0F, 0x52,
    0x10, 0xD8, 0x00, 0x00, 0x36, 0x02, 0xA6, 0x00, 0x82, 0xEA, 0x14, 0xC9, 0xE9, 0x63, 0x76, 0x33,
    0x7A, 0x75, 0x7A, 0x75, 0xF7, 0xA2, 0xD5, 0x5C, 0x19, 0xF7, 0x4D, 0xFF, 0xE9, 0x30, 0xF8, 0x80
};
// clang-format on

static struct SimulatedSpiNorFlashMedium* medium;
static uint32_t (*milliseconds)(void);

static bool selected;

/* The first byte sent since the part was selected, how many have been, and
 * the address the three after the command gave, which then moves on with
 * each byte read. */
static uint8_t command;
static size_t received;
static uint32_t address;

static inline void SimulatedSpiNorFlash_TakeByte(uint8_t byte);
static inline bool SimulatedSpiNorFlash_IsAsked(uint8_t asked, size_t requestBytes);
static bool SimulatedSpiNorFlash_ReadData(uint8_t* bytes, size_t count);
static inline size_t SimulatedSpiNorFlash_BytesToEndOfPart(size_t remaining);
static void SimulatedSpiNorFlash_ReadSfdp(uint8_t* bytes, size_t count);
static inline uint8_t SimulatedSpiNorFlash_SfdpAt(uint32_t sfdpAddress);

struct SolidSyslogSpiNorFlashBus* SimulatedSpiNorFlash_PowerUp(const struct SimulatedSpiNorFlashConfig* config)
{
    medium = config->Medium;
    milliseconds = config->Milliseconds;
    selected = false;
    received = 0U;
    return &bus;
}

static void SimulatedSpiNorFlash_Select(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    selected = true;
    received = 0U;
}

static void SimulatedSpiNorFlash_Deselect(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    selected = false;
    received = 0U;
}

static bool SimulatedSpiNorFlash_Write(struct SolidSyslogSpiNorFlashBus* base, const void* buffer, size_t count)
{
    (void) base;
    const uint8_t* bytes = (const uint8_t*) buffer;
    for (size_t i = 0U; selected && (i < count); i++)
    {
        SimulatedSpiNorFlash_TakeByte(bytes[i]);
    }
    return true;
}

static inline void SimulatedSpiNorFlash_TakeByte(uint8_t byte)
{
    if (received == 0U)
    {
        command = byte;
        address = 0U;
    }
    else if (received <= (size_t) ADDRESS_BYTES)
    {
        address = (address << 8U) | byte;
    }
    else
    {
        /* A byte the command has no use for. */
    }
    received++;
}

static bool SimulatedSpiNorFlash_Read(struct SolidSyslogSpiNorFlashBus* base, void* buffer, size_t count)
{
    (void) base;
    uint8_t* bytes = (uint8_t*) buffer;
    bool read = true;
    if (SimulatedSpiNorFlash_IsAsked(READ_DATA, COMMAND_AND_ADDRESS_BYTES))
    {
        read = SimulatedSpiNorFlash_ReadData(bytes, count);
    }
    else if (SimulatedSpiNorFlash_IsAsked(READ_SFDP, SFDP_REQUEST_BYTES))
    {
        SimulatedSpiNorFlash_ReadSfdp(bytes, count);
    }
    else if (SimulatedSpiNorFlash_IsAsked(READ_STATUS, COMMAND_BYTES))
    {
        (void) memset(bytes, 0x00, count);
    }
    else
    {
        (void) memset(bytes, IDLE_LINE, count);
    }
    return read;
}

/* Whether the part has been sent that command and as many bytes as it takes
 * before it answers. */
static inline bool SimulatedSpiNorFlash_IsAsked(uint8_t asked, size_t requestBytes)
{
    return selected && (command == asked) && (received >= requestBytes);
}

/* One call of the medium, or two for a read that runs past the end of the
 * part and so goes round to its start. */
static bool SimulatedSpiNorFlash_ReadData(uint8_t* bytes, size_t count)
{
    size_t done = 0U;
    bool read = true;
    while (read && (done < count))
    {
        size_t length = SimulatedSpiNorFlash_BytesToEndOfPart(count - done);
        read = medium->Read(medium, address, &bytes[done], length);
        address = (address + (uint32_t) length) % (uint32_t) PART_BYTES;
        done += length;
    }
    for (size_t i = 0U; read && (i < count); i++)
    {
        bytes[i] = (uint8_t) ~bytes[i];
    }
    return read;
}

static inline size_t SimulatedSpiNorFlash_BytesToEndOfPart(size_t remaining)
{
    size_t toEnd = (size_t) PART_BYTES - (size_t) address;
    return (remaining < toEnd) ? remaining : toEnd;
}

static void SimulatedSpiNorFlash_ReadSfdp(uint8_t* bytes, size_t count)
{
    for (size_t i = 0U; i < count; i++)
    {
        bytes[i] = SimulatedSpiNorFlash_SfdpAt(address);
        address++;
    }
}

/* Every byte outside the header and the table reads as erased. */
static inline uint8_t SimulatedSpiNorFlash_SfdpAt(uint32_t sfdpAddress)
{
    uint32_t offset = sfdpAddress % (uint32_t) SFDP_BYTES;
    uint8_t byte = IDLE_LINE;
    if (offset < (uint32_t) SFDP_HEADER_BYTES)
    {
        byte = SFDP_HEADER[offset];
    }
    else if ((offset >= (uint32_t) SFDP_TABLE_ADDRESS) && (offset < (uint32_t) (SFDP_TABLE_ADDRESS + SFDP_TABLE_BYTES)))
    {
        byte = SFDP_TABLE[offset - (uint32_t) SFDP_TABLE_ADDRESS];
    }
    else
    {
        /* Erased. */
    }
    return byte;
}

static uint32_t SimulatedSpiNorFlash_Milliseconds(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    return milliseconds();
}
