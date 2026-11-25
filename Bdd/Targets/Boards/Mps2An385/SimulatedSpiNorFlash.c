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
    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    SECTOR_ERASE = 0x20,
    READ_SFDP = 0x5A,

    STATUS_BUSY = 0x01,
    STATUS_WRITE_ENABLED = 0x02,

    /* What a line with nothing driving it reads as. */
    IDLE_LINE = 0xFF,

    /* A W25Q128JV's. */
    PART_BYTES = 16 * 1024 * 1024,
    PAGE_BYTES = 256,
    /* Its smallest erase, which is the one its SFDP names. */
    SECTOR_BYTES = 4096,

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
static bool writeEnabled;

/* Whether a program or an erase has begun and not yet been seen to finish,
 * with the count as it began and how long it takes. A start and a length,
 * not the count it ends at, so that being busy holds across a wrap of the
 * count. */
static bool writing;
static uint32_t writingSince;
static uint32_t writingFor;

/* The first byte sent since the part was selected, how many have been, and
 * the address the three after the command gave, which then moves on with
 * each byte read. */
static uint8_t command;
static size_t received;
static uint32_t address;

/* What a program command has sent to be programmed, each byte at its place
 * in the page, and how many bytes that was. */
static uint8_t page[PAGE_BYTES];
static size_t dataCount;

static bool SimulatedSpiNorFlash_IsBusy(void);
static inline bool SimulatedSpiNorFlash_IsAsked(uint8_t asked, size_t requestBytes);
static void SimulatedSpiNorFlash_ProgramThePage(void);
static void SimulatedSpiNorFlash_ProgramInPage(uint32_t pageAddress, size_t from, size_t count);
static inline void SimulatedSpiNorFlash_BeBusyFor(uint32_t time);
static void SimulatedSpiNorFlash_EraseTheSector(void);
static inline void SimulatedSpiNorFlash_TakeByte(uint8_t byte);
static inline uint8_t SimulatedSpiNorFlash_Status(void);
static bool SimulatedSpiNorFlash_ReadData(uint8_t* bytes, size_t count);
static inline size_t SimulatedSpiNorFlash_BytesToEndOfPart(size_t remaining);
static void SimulatedSpiNorFlash_ReadSfdp(uint8_t* bytes, size_t count);
static inline uint8_t SimulatedSpiNorFlash_SfdpAt(uint32_t sfdpAddress);

struct SolidSyslogSpiNorFlashBus* SimulatedSpiNorFlash_PowerUp(const struct SimulatedSpiNorFlashConfig* config)
{
    medium = config->Medium;
    milliseconds = config->Milliseconds;
    selected = false;
    writeEnabled = false;
    writing = false;
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
    if (SimulatedSpiNorFlash_IsBusy())
    {
        /* It takes nothing until it has finished. */
    }
    else if (SimulatedSpiNorFlash_IsAsked(WRITE_ENABLE, COMMAND_BYTES))
    {
        writeEnabled = true;
    }
    else if (SimulatedSpiNorFlash_IsAsked(PAGE_PROGRAM, COMMAND_AND_ADDRESS_BYTES + 1U) && writeEnabled)
    {
        SimulatedSpiNorFlash_ProgramThePage();
        SimulatedSpiNorFlash_BeBusyFor(SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS);
    }
    else if (SimulatedSpiNorFlash_IsAsked(SECTOR_ERASE, COMMAND_AND_ADDRESS_BYTES) && writeEnabled)
    {
        SimulatedSpiNorFlash_EraseTheSector();
        SimulatedSpiNorFlash_BeBusyFor(SIMULATED_SPI_NOR_FLASH_ERASE_MILLISECONDS);
    }
    else
    {
        /* Nothing the part acts on as its select rises. */
    }
    selected = false;
    received = 0U;
}

/* Writing stays enabled until what was begun has finished, and the part
 * disables it then. Nothing happens in the part between two calls of its
 * bus, so the finishing is seen at the first call after its time. */
static bool SimulatedSpiNorFlash_IsBusy(void)
{
    if (writing && ((milliseconds() - writingSince) >= writingFor))
    {
        writing = false;
        writeEnabled = false;
    }
    return writing;
}

/* Whether the part has been sent that command and as many bytes as it takes
 * before it answers. */
static inline bool SimulatedSpiNorFlash_IsAsked(uint8_t asked, size_t requestBytes)
{
    return selected && (command == asked) && (received >= requestBytes);
}

/* What was sent goes into the page from the address given, and round to the
 * page's start where it ran past the end. More than a page of it has gone
 * over itself, so the whole page is programmed with what was sent last. */
static void SimulatedSpiNorFlash_ProgramThePage(void)
{
    uint32_t pageAddress = address - (address % (uint32_t) PAGE_BYTES);
    size_t from = (size_t) (address % (uint32_t) PAGE_BYTES);
    size_t count = (dataCount < (size_t) PAGE_BYTES) ? dataCount : (size_t) PAGE_BYTES;
    size_t toEndOfPage = (size_t) PAGE_BYTES - from;
    size_t first = (count < toEndOfPage) ? count : toEndOfPage;

    SimulatedSpiNorFlash_ProgramInPage(pageAddress, from, first);
    SimulatedSpiNorFlash_ProgramInPage(pageAddress, 0U, count - first);
}

/* A program clears bits and sets none. The medium holds each byte's
 * complement, so there it sets bits and clears none. A medium that fails
 * leaves the part as it was, and the part does not say so: what is read back
 * shows it. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- where in the page, and how much of it; distinct semantics
static void SimulatedSpiNorFlash_ProgramInPage(uint32_t pageAddress, size_t from, size_t count)
{
    uint8_t kept[PAGE_BYTES];
    if ((count > 0U) && medium->Read(medium, pageAddress + (uint32_t) from, kept, count))
    {
        for (size_t i = 0U; i < count; i++)
        {
            kept[i] |= (uint8_t) ~page[from + i];
        }
        (void) medium->Write(medium, pageAddress + (uint32_t) from, kept, count);
    }
}

static inline void SimulatedSpiNorFlash_BeBusyFor(uint32_t time)
{
    writing = true;
    writingSince = milliseconds();
    writingFor = time;
}

/* The sector the address is in, whole. Erased is all ones, which the medium
 * holds as zeros. A medium that fails leaves the part as it was. */
static void SimulatedSpiNorFlash_EraseTheSector(void)
{
    static const uint8_t ERASED_SECTOR[SECTOR_BYTES] = {0U};
    uint32_t sectorAddress = address - (address % (uint32_t) SECTOR_BYTES);

    (void) medium->Write(medium, sectorAddress, ERASED_SECTOR, sizeof(ERASED_SECTOR));
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
        dataCount = 0U;
    }
    else if (received <= (size_t) ADDRESS_BYTES)
    {
        address = (address << 8U) | byte;
    }
    else if (command == (uint8_t) PAGE_PROGRAM)
    {
        page[(address + (uint32_t) dataCount) % (uint32_t) PAGE_BYTES] = byte;
        dataCount++;
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
    if (SimulatedSpiNorFlash_IsAsked(READ_STATUS, COMMAND_BYTES))
    {
        (void) memset(bytes, SimulatedSpiNorFlash_Status(), count);
    }
    else if (SimulatedSpiNorFlash_IsBusy())
    {
        /* It answers nothing else until it has finished. */
        (void) memset(bytes, IDLE_LINE, count);
    }
    else if (SimulatedSpiNorFlash_IsAsked(READ_DATA, COMMAND_AND_ADDRESS_BYTES))
    {
        read = SimulatedSpiNorFlash_ReadData(bytes, count);
    }
    else if (SimulatedSpiNorFlash_IsAsked(READ_SFDP, SFDP_REQUEST_BYTES))
    {
        SimulatedSpiNorFlash_ReadSfdp(bytes, count);
    }
    else
    {
        (void) memset(bytes, IDLE_LINE, count);
    }
    return read;
}

static inline uint8_t SimulatedSpiNorFlash_Status(void)
{
    uint8_t status = SimulatedSpiNorFlash_IsBusy() ? (uint8_t) STATUS_BUSY : 0U;
    if (writeEnabled)
    {
        status |= (uint8_t) STATUS_WRITE_ENABLED;
    }
    return status;
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
