/* An SPI NOR flash - see BddTargetSpiNorFlash.h.
 *
 * The commands and the times are those of the Winbond W25Q128JV datasheet. */

#include "BddTargetSpiNorFlash.h"

#include "BddTargetBoardFlashBus.h"

#include <stddef.h>

enum
{
    /* A line with nothing driving it. */
    IDLE_LINE = 0xFF,
    /* Clocked out during a read. The part ignores it. */
    READ_FILL = 0x00,

    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    SECTOR_ERASE = 0x20,
    READ_JEDEC_ID = 0x9F,

    STATUS_BUSY = 0x01,
    STATUS_WRITE_ENABLED = 0x02,

    /* The maker, the memory type, and the capacity as a power of two. */
    JEDEC_ID_LENGTH = 3,
    JEDEC_MAKER = 0,
    JEDEC_TYPE = 1,
    JEDEC_CAPACITY = 2,
    MAKER_WINBOND = 0xEF,
    /* The W25Q family, which takes these commands in plain SPI as supplied.
     * Winbond's other families do not all erase by this sector size or with
     * this command. */
    TYPE_W25Q = 0x40,
    /* 64 KiB, the smallest of the family, to 16 MiB, the largest a
     * three-byte address reaches. */
    SMALLEST_CAPACITY_CODE = 0x10,
    LARGEST_CAPACITY_CODE = 0x18,

    /* The part finishes a sector erase within 400 ms and a page program
     * within 3 ms. Each is given more than twice that. An erase is also the
     * longest thing a reset can have interrupted. */
    ERASE_TIMEOUT_MILLISECONDS = 1000,
    PROGRAM_TIMEOUT_MILLISECONDS = 10
};

static bool started;
static uint32_t capacity;

static enum BddTargetSpiNorFlashResult SpiNorFlash_WaitUntilReady(uint32_t milliseconds);
static inline uint8_t SpiNorFlash_Status(void);
static inline bool SpiNorFlash_IsBusy(uint8_t status);
static inline bool SpiNorFlash_HasPassed(uint32_t since, uint32_t milliseconds);
static enum BddTargetSpiNorFlashResult SpiNorFlash_Identify(void);
static inline bool SpiNorFlash_IsNoAnswer(const uint8_t* id);
static inline bool SpiNorFlash_IsSupported(const uint8_t* id);
static enum BddTargetSpiNorFlashResult SpiNorFlash_CheckSpan(uint32_t address, size_t count);
static inline bool SpiNorFlash_IsSpanInThePart(uint32_t address, size_t count);
static inline void SpiNorFlash_Begin(uint8_t command, uint32_t address);
static inline size_t SpiNorFlash_BytesLeftInPage(uint32_t address);
static enum BddTargetSpiNorFlashResult SpiNorFlash_ProgramInPage(uint32_t address, const uint8_t* bytes, size_t count);
static enum BddTargetSpiNorFlashResult SpiNorFlash_WaitForTheWrite(uint32_t milliseconds);
static enum BddTargetSpiNorFlashResult SpiNorFlash_EnableWriting(void);
static enum BddTargetSpiNorFlashResult SpiNorFlash_CheckSector(uint32_t sector);

const char* BddTargetSpiNorFlash_ResultText(enum BddTargetSpiNorFlashResult result)
{
    const char* text = "ok";
    switch (result)
    {
        case BDD_TARGET_SPI_NOR_FLASH_OK:
            break;
        case BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED:
            text = "not started";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE:
            text = "out of range";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER:
            text = "no answer";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED:
            text = "not a part this drives";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT:
            text = "timed out";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED:
            text = "would not enable writing";
            break;
        default:
            text = "unknown";
            break;
    }
    return text;
}

enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Start(void)
{
    BddTargetSpiNorFlash_Stop();
    BddTargetBoardFlashBus_Take();
    /* A part that was erasing when the processor was reset carries on, and
     * until it finishes it answers only a status read. */
    enum BddTargetSpiNorFlashResult result = SpiNorFlash_WaitUntilReady(ERASE_TIMEOUT_MILLISECONDS);
    if (result == BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        result = SpiNorFlash_Identify();
    }
    started = (result == BDD_TARGET_SPI_NOR_FLASH_OK);
    return result;
}

static enum BddTargetSpiNorFlashResult SpiNorFlash_WaitUntilReady(uint32_t milliseconds)
{
    enum BddTargetSpiNorFlashResult result = BDD_TARGET_SPI_NOR_FLASH_OK;
    uint32_t since = BddTargetBoardFlashBus_Milliseconds();
    uint8_t status = SpiNorFlash_Status();
    bool expired = false;
    while (SpiNorFlash_IsBusy(status) && !expired)
    {
        /* The time is read before the status, so the status is always read
         * once after the timeout has passed. Otherwise a task switched out
         * for longer than the timeout would give up on a part that finished
         * while it was away. */
        expired = SpiNorFlash_HasPassed(since, milliseconds);
        status = SpiNorFlash_Status();
    }
    if (status == (uint8_t) IDLE_LINE)
    {
        result = BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER;
    }
    else if (SpiNorFlash_IsBusy(status))
    {
        result = BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT;
    }
    else
    {
        /* Ready. */
    }
    return result;
}

static inline uint8_t SpiNorFlash_Status(void)
{
    BddTargetBoardFlashBus_Select();
    (void) BddTargetBoardFlashBus_Exchange(READ_STATUS);
    uint8_t status = BddTargetBoardFlashBus_Exchange(READ_FILL);
    BddTargetBoardFlashBus_Deselect();
    return status;
}

/* A status of all ones is an undriven line and not a busy part: no part is
 * busy with every protection bit set as well. */
static inline bool SpiNorFlash_IsBusy(uint8_t status)
{
    return (status != (uint8_t) IDLE_LINE) && ((status & (uint8_t) STATUS_BUSY) != 0U);
}

/* Unsigned arithmetic, so it holds when the count wraps. */
static inline bool SpiNorFlash_HasPassed(uint32_t since, uint32_t milliseconds)
{
    return (BddTargetBoardFlashBus_Milliseconds() - since) >= milliseconds;
}

static enum BddTargetSpiNorFlashResult SpiNorFlash_Identify(void)
{
    enum BddTargetSpiNorFlashResult result = BDD_TARGET_SPI_NOR_FLASH_OK;
    uint8_t id[JEDEC_ID_LENGTH] = {0};
    BddTargetBoardFlashBus_Select();
    (void) BddTargetBoardFlashBus_Exchange(READ_JEDEC_ID);
    for (size_t i = 0U; i < (size_t) JEDEC_ID_LENGTH; i++)
    {
        id[i] = BddTargetBoardFlashBus_Exchange(READ_FILL);
    }
    BddTargetBoardFlashBus_Deselect();

    if (SpiNorFlash_IsNoAnswer(id))
    {
        result = BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER;
    }
    else if (!SpiNorFlash_IsSupported(id))
    {
        result = BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED;
    }
    else
    {
        capacity = (uint32_t) 1U << id[JEDEC_CAPACITY];
    }
    return result;
}

/* An undriven line reads as all ones, and one held low as all zeros.
 * Neither is a manufacturer's code. */
static inline bool SpiNorFlash_IsNoAnswer(const uint8_t* id)
{
    return (id[JEDEC_MAKER] == (uint8_t) IDLE_LINE) || (id[JEDEC_MAKER] == 0U);
}

static inline bool SpiNorFlash_IsSupported(const uint8_t* id)
{
    return (id[JEDEC_MAKER] == (uint8_t) MAKER_WINBOND) && (id[JEDEC_TYPE] == (uint8_t) TYPE_W25Q) &&
           (id[JEDEC_CAPACITY] >= (uint8_t) SMALLEST_CAPACITY_CODE) &&
           (id[JEDEC_CAPACITY] <= (uint8_t) LARGEST_CAPACITY_CODE);
}

void BddTargetSpiNorFlash_Stop(void)
{
    started = false;
    capacity = 0U;
}

bool BddTargetSpiNorFlash_IsStarted(void)
{
    return started;
}

uint32_t BddTargetSpiNorFlash_Capacity(void)
{
    return capacity;
}

enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Read(uint32_t address, uint8_t* buffer, size_t count)
{
    enum BddTargetSpiNorFlashResult result = SpiNorFlash_CheckSpan(address, count);
    if (result == BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        SpiNorFlash_Begin(READ_DATA, address);
        for (size_t i = 0U; i < count; i++)
        {
            buffer[i] = BddTargetBoardFlashBus_Exchange(READ_FILL);
        }
        BddTargetBoardFlashBus_Deselect();
        /* Erased bytes and an undriven line both read as all ones, so the
         * data cannot show whether the part sent it. The status can. */
        if (SpiNorFlash_Status() == (uint8_t) IDLE_LINE)
        {
            result = BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER;
        }
    }
    return result;
}

static enum BddTargetSpiNorFlashResult SpiNorFlash_CheckSpan(uint32_t address, size_t count)
{
    enum BddTargetSpiNorFlashResult result = BDD_TARGET_SPI_NOR_FLASH_OK;
    if (!started)
    {
        result = BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED;
    }
    else if (!SpiNorFlash_IsSpanInThePart(address, count))
    {
        result = BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE;
    }
    else
    {
        /* All of it is in the part. */
    }
    return result;
}

/* The subtraction keeps `address + count` from wrapping past the comparison. */
static inline bool SpiNorFlash_IsSpanInThePart(uint32_t address, size_t count)
{
    return (count <= (size_t) capacity) && (address <= (capacity - (uint32_t) count));
}

/* Selects the part and sends a command that takes an address, most
 * significant byte first. The caller deselects. */
static inline void SpiNorFlash_Begin(uint8_t command, uint32_t address)
{
    BddTargetBoardFlashBus_Select();
    (void) BddTargetBoardFlashBus_Exchange(command);
    (void) BddTargetBoardFlashBus_Exchange((uint8_t) (address >> 16U));
    (void) BddTargetBoardFlashBus_Exchange((uint8_t) (address >> 8U));
    (void) BddTargetBoardFlashBus_Exchange((uint8_t) address);
}

enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Program(uint32_t address, const uint8_t* bytes, size_t count)
{
    enum BddTargetSpiNorFlashResult result = SpiNorFlash_CheckSpan(address, count);
    size_t done = 0U;
    while ((done < count) && (result == BDD_TARGET_SPI_NOR_FLASH_OK))
    {
        uint32_t from = address + (uint32_t) done;
        size_t inPage = SpiNorFlash_BytesLeftInPage(from);
        size_t length = ((count - done) < inPage) ? (count - done) : inPage;
        result = SpiNorFlash_ProgramInPage(from, &bytes[done], length);
        done += length;
    }
    return result;
}

/* A program that runs past the end of its page wraps to the page's start,
 * so no more than this is sent in one command. */
static inline size_t SpiNorFlash_BytesLeftInPage(uint32_t address)
{
    return (size_t) BDD_TARGET_SPI_NOR_FLASH_PAGE_SIZE -
           (size_t) (address % (uint32_t) BDD_TARGET_SPI_NOR_FLASH_PAGE_SIZE);
}

static enum BddTargetSpiNorFlashResult SpiNorFlash_ProgramInPage(uint32_t address, const uint8_t* bytes, size_t count)
{
    enum BddTargetSpiNorFlashResult result = SpiNorFlash_EnableWriting();
    if (result == BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        SpiNorFlash_Begin(PAGE_PROGRAM, address);
        for (size_t i = 0U; i < count; i++)
        {
            (void) BddTargetBoardFlashBus_Exchange(bytes[i]);
        }
        /* The part begins the program as its select rises. */
        BddTargetBoardFlashBus_Deselect();
        result = SpiNorFlash_WaitForTheWrite(PROGRAM_TIMEOUT_MILLISECONDS);
    }
    return result;
}

/* A part still busy after the timeout answers only a status read, and a
 * data read would appear to succeed with nothing sent back. So it is stopped;
 * the Start that brings it back waits for it. */
static enum BddTargetSpiNorFlashResult SpiNorFlash_WaitForTheWrite(uint32_t milliseconds)
{
    enum BddTargetSpiNorFlashResult result = SpiNorFlash_WaitUntilReady(milliseconds);
    if (result == BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT)
    {
        BddTargetSpiNorFlash_Stop();
    }
    return result;
}

/* The part accepts a program or an erase only immediately after a write
 * enable, and its status shows whether the enable took. If it did not, the
 * part would ignore the next command without reporting it. */
static enum BddTargetSpiNorFlashResult SpiNorFlash_EnableWriting(void)
{
    enum BddTargetSpiNorFlashResult result = BDD_TARGET_SPI_NOR_FLASH_OK;
    BddTargetBoardFlashBus_Select();
    (void) BddTargetBoardFlashBus_Exchange(WRITE_ENABLE);
    BddTargetBoardFlashBus_Deselect();
    uint8_t status = SpiNorFlash_Status();
    if (status == (uint8_t) IDLE_LINE)
    {
        result = BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER;
    }
    else if ((status & (uint8_t) STATUS_WRITE_ENABLED) == 0U)
    {
        result = BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED;
    }
    else
    {
        /* Writing is enabled. */
    }
    return result;
}

enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_EraseSector(uint32_t sector)
{
    enum BddTargetSpiNorFlashResult result = SpiNorFlash_CheckSector(sector);
    if (result == BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        result = SpiNorFlash_EnableWriting();
    }
    if (result == BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        SpiNorFlash_Begin(SECTOR_ERASE, sector * (uint32_t) BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE);
        /* The part begins the erase as its select rises. */
        BddTargetBoardFlashBus_Deselect();
        result = SpiNorFlash_WaitForTheWrite(ERASE_TIMEOUT_MILLISECONDS);
    }
    return result;
}

static enum BddTargetSpiNorFlashResult SpiNorFlash_CheckSector(uint32_t sector)
{
    enum BddTargetSpiNorFlashResult result = BDD_TARGET_SPI_NOR_FLASH_OK;
    if (!started)
    {
        result = BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED;
    }
    else if (sector >= (capacity / (uint32_t) BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE))
    {
        result = BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE;
    }
    else
    {
        /* It is a sector of the part. */
    }
    return result;
}
