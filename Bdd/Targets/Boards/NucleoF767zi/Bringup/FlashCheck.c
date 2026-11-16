/* The flash check of the bring-up image - see FlashCheck.h. */

#include "FlashCheck.h"

#include "BddTargetSpiNorFlash.h"
#include "Board.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum
{
    LINE_BYTES = 128,
    ERASED = 0xFF,

    /* Starts in the sector's first page and runs into its second, so the
     * driver has to split the program. */
    SPLIT_OFFSET = 200,
    SPLIT_BYTES = 300,

    /* Two adjacent programs within one page, as a filesystem that programs
     * less than a page at a time makes. */
    PIECE_OFFSET = 1024,
    PIECE_BYTES = 16
};

static uint8_t expected[BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE];
static uint8_t found[BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE];

static void FlashCheck_Say(const char* format, ...) __attribute__((format(printf, 1, 2)));
static bool FlashCheck_Erase(uint32_t sector);
static bool FlashCheck_Passed(const char* what, enum BddTargetSpiNorFlashResult result);
static bool FlashCheck_HoldsWhatIsExpected(const char* what, uint32_t sector);
static bool FlashCheck_Program(const char* what, uint32_t sector, size_t offset, size_t count);
static inline void FlashCheck_FillPattern(uint8_t* bytes, size_t count);

void FlashCheck_Run(void)
{
    enum BddTargetSpiNorFlashResult started = BddTargetSpiNorFlash_Start();
    bool passed = FlashCheck_Passed("start", started);
    if (passed)
    {
        uint32_t capacity = BddTargetSpiNorFlash_Capacity();
        uint32_t sector = (capacity / (uint32_t) BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE) - 1U;
        FlashCheck_Say("started, %lu bytes, checking sector %lu", (unsigned long) capacity, (unsigned long) sector);
        passed = FlashCheck_Erase(sector) &&
                 FlashCheck_Program("a program across two pages", sector, SPLIT_OFFSET, SPLIT_BYTES) &&
                 FlashCheck_Program("a program of part of a page", sector, PIECE_OFFSET, PIECE_BYTES) &&
                 FlashCheck_Program("a second beside it", sector, PIECE_OFFSET + PIECE_BYTES, PIECE_BYTES);
        /* Erased again whatever the result, so the check leaves nothing
         * behind. */
        passed = FlashCheck_Erase(sector) && passed;
    }
    FlashCheck_Say(passed ? "passed" : "failed");
}

/* One line in one write. */
static void FlashCheck_Say(const char* format, ...)
{
    static const char LINE_START[] = "flash: ";
    char text[LINE_BYTES];
    size_t length = sizeof(LINE_START) - 1U;
    va_list arguments;
    (void) memcpy(text, LINE_START, length);
    va_start(arguments, format);
    int formatted = vsnprintf(&text[length], sizeof(text) - length, format, arguments);
    va_end(arguments);
    /* A line that leaves no room for its line ending is dropped. */
    if ((formatted > 0) && (((size_t) formatted + length) < (sizeof(text) - 2U)))
    {
        length += (size_t) formatted;
        text[length] = '\r';
        text[length + 1U] = '\n';
        Board_ConsoleWrite(text, length + 2U);
    }
}

static bool FlashCheck_Passed(const char* what, enum BddTargetSpiNorFlashResult result)
{
    if (result != BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        FlashCheck_Say("%s: %s", what, BddTargetSpiNorFlash_ResultText(result));
    }
    return result == BDD_TARGET_SPI_NOR_FLASH_OK;
}

static bool FlashCheck_Erase(uint32_t sector)
{
    uint32_t since = Board_Milliseconds();
    bool passed = FlashCheck_Passed("erase", BddTargetSpiNorFlash_EraseSector(sector));
    uint32_t took = Board_Milliseconds() - since;
    (void) memset(expected, ERASED, sizeof(expected));
    passed = passed && FlashCheck_HoldsWhatIsExpected("an erase", sector);
    if (passed)
    {
        FlashCheck_Say("erased the sector in %lu ms, and it reads as erased", (unsigned long) took);
    }
    return passed;
}

/* The whole sector is read back, so a program or an erase that touched bytes
 * outside its range is caught as well as one that missed its own. */
static bool FlashCheck_HoldsWhatIsExpected(const char* what, uint32_t sector)
{
    uint32_t address = sector * (uint32_t) BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE;
    bool passed = FlashCheck_Passed("read", BddTargetSpiNorFlash_Read(address, found, sizeof(found)));
    if (passed)
    {
        size_t differs = 0U;
        while ((differs < sizeof(found)) && (found[differs] == expected[differs]))
        {
            differs++;
        }
        passed = (differs == sizeof(found));
        if (!passed)
        {
            FlashCheck_Say(
                "after %s, byte %lu of the sector reads %02X and not %02X",
                what,
                (unsigned long) differs,
                (unsigned) found[differs],
                (unsigned) expected[differs]
            );
        }
    }
    return passed;
}

static bool FlashCheck_Program(const char* what, uint32_t sector, size_t offset, size_t count)
{
    uint32_t address = (sector * (uint32_t) BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE) + (uint32_t) offset;
    FlashCheck_FillPattern(&expected[offset], count);
    uint32_t since = Board_Milliseconds();
    bool passed = FlashCheck_Passed(what, BddTargetSpiNorFlash_Program(address, &expected[offset], count));
    uint32_t took = Board_Milliseconds() - since;
    passed = passed && FlashCheck_HoldsWhatIsExpected(what, sector);
    if (passed)
    {
        FlashCheck_Say(
            "%s, %lu bytes in %lu ms, and the sector reads back",
            what,
            (unsigned long) count,
            (unsigned long) took
        );
    }
    return passed;
}

/* No two adjacent bytes are equal. */
static inline void FlashCheck_FillPattern(uint8_t* bytes, size_t count)
{
    for (size_t i = 0U; i < count; i++)
    {
        bytes[i] = (uint8_t) (0xA5U ^ (uint8_t) i);
    }
}
