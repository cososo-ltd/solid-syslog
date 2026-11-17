/* The NUCLEO-F767ZI's disk behind the board seam - see BddTargetBoardDisk.h.
 *
 * It is the first BDD_TARGET_BOARD_DISK_SECTOR_COUNT sectors of a MicroSD
 * card on SPI4, driven by BddTargetSdCard.c over the bus in
 * BddTargetBoardSdCardBus.c. With no card fitted the disk never becomes
 * ready, a mount over it fails, and the target stays on the store it started
 * with.
 *
 * The card is on a breadboard, on wires that can work loose, so when a sector
 * will not transfer the card is started again and the sector tried once more
 * before the disk reports an error.
 *
 * Failures are written to the console, so that a card that has stopped
 * answering can be told apart from a fault in the store. */

#include "BddTargetBoardDisk.h"

#include "BddTargetSdCard.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

enum
{
    LINE_BYTES = 128
};

static const char LINE_START[] = "[board] disk: ";

static bool ready;
/* What EnsureReady last reported, so that a mount retried over a missing card
 * reports it once. */
static enum BddTargetSdCardResult startFailureSaid = BDD_TARGET_SD_CARD_OK;
static bool tooSmallSaid;
static uint32_t repeatsSaid;

static void BddTargetBoardDisk_Say(const char* format, ...) __attribute__((format(printf, 1, 2)));
static inline const char* BddTargetBoardDisk_Reason(enum BddTargetSdCardResult result);
static bool BddTargetBoardDisk_IsRangeValid(uint32_t sector, uint32_t count);
static enum BddTargetSdCardResult BddTargetBoardDisk_ReadSector(uint32_t sector, uint8_t* bytes);
static void BddTargetBoardDisk_SayRepeats(const char* what, uint32_t sector);
static bool BddTargetBoardDisk_StartAgain(const char* what, uint32_t sector, enum BddTargetSdCardResult result);
static enum BddTargetSdCardResult BddTargetBoardDisk_WriteSector(uint32_t sector, const uint8_t* bytes);

bool BddTargetBoardDisk_EnsureReady(void)
{
    if (!ready)
    {
        enum BddTargetSdCardResult result = BddTargetSdCard_Start();
        repeatsSaid = 0U;
        if (result != BDD_TARGET_SD_CARD_OK)
        {
            if (result != startFailureSaid)
            {
                BddTargetBoardDisk_Say("the card did not start: %s", BddTargetBoardDisk_Reason(result));
            }
        }
        else if (BddTargetSdCard_SectorCount() < (uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT)
        {
            if (!tooSmallSaid)
            {
                BddTargetBoardDisk_Say("the card is smaller than the disk");
                tooSmallSaid = true;
            }
            BddTargetSdCard_Stop();
        }
        else
        {
            ready = true;
        }
        startFailureSaid = result;
    }
    return ready;
}

/* One line in one write, which the console lock keeps whole against another
 * task's output. */
static void BddTargetBoardDisk_Say(const char* format, ...)
{
    char line[LINE_BYTES];
    size_t length = sizeof(LINE_START) - 1U;
    va_list arguments;
    (void) snprintf(line, sizeof(line), "%s", LINE_START);
    va_start(arguments, format);
    int formatted = vsnprintf(&line[length], sizeof(line) - length, format, arguments);
    va_end(arguments);
    /* A line that leaves no room for the newline is dropped. */
    if ((formatted > 0) && (((size_t) formatted + length) < (sizeof(line) - 1U)))
    {
        length += (size_t) formatted;
        line[length] = '\n';
        (void) write(STDOUT_FILENO, line, length + 1U);
    }
}

static inline const char* BddTargetBoardDisk_Reason(enum BddTargetSdCardResult result)
{
    const char* reason = "ok";
    switch (result)
    {
        case BDD_TARGET_SD_CARD_OK:
            break;
        case BDD_TARGET_SD_CARD_NOT_STARTED:
            reason = "not started";
            break;
        case BDD_TARGET_SD_CARD_NO_ANSWER:
            reason = "no answer";
            break;
        case BDD_TARGET_SD_CARD_UNSUPPORTED:
            reason = "not a card this drives";
            break;
        case BDD_TARGET_SD_CARD_REFUSED:
            reason = "refused";
            break;
        case BDD_TARGET_SD_CARD_TIMED_OUT:
            reason = "timed out";
            break;
        case BDD_TARGET_SD_CARD_CRC_FAILED:
            reason = "CRC failed";
            break;
        case BDD_TARGET_SD_CARD_WRITE_FAILED:
            reason = "write failed";
            break;
        default:
            reason = "unknown";
            break;
    }
    return reason;
}

bool BddTargetBoardDisk_IsReady(void)
{
    return ready;
}

enum BddTargetBoardDiskResult BddTargetBoardDisk_Read(void* buffer, uint32_t sector, uint32_t count)
{
    enum BddTargetBoardDiskResult result = BDD_TARGET_BOARD_DISK_OK;
    if (!BddTargetBoardDisk_IsRangeValid(sector, count))
    {
        result = BDD_TARGET_BOARD_DISK_OUT_OF_RANGE;
    }
    else
    {
        uint8_t* bytes = buffer;
        for (uint32_t i = 0U; (i < count) && (result == BDD_TARGET_BOARD_DISK_OK); i++)
        {
            uint8_t* into = &bytes[(size_t) i * (size_t) BDD_TARGET_BOARD_DISK_SECTOR_SIZE];
            if (BddTargetBoardDisk_ReadSector(sector + i, into) != BDD_TARGET_SD_CARD_OK)
            {
                result = BDD_TARGET_BOARD_DISK_IO_ERROR;
            }
        }
    }
    return result;
}

/* Written as a subtraction so that `sector + count` cannot wrap. */
static bool BddTargetBoardDisk_IsRangeValid(uint32_t sector, uint32_t count)
{
    return (count <= (uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT) &&
           (sector <= ((uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT - count));
}

static enum BddTargetSdCardResult BddTargetBoardDisk_ReadSector(uint32_t sector, uint8_t* bytes)
{
    enum BddTargetSdCardResult result = BddTargetSdCard_Read(sector, bytes);
    BddTargetBoardDisk_SayRepeats("read", sector);
    if ((result != BDD_TARGET_SD_CARD_OK) && BddTargetBoardDisk_StartAgain("read", sector, result))
    {
        result = BddTargetSdCard_Read(sector, bytes);
        BddTargetBoardDisk_SayRepeats("read", sector);
    }
    if (result != BDD_TARGET_SD_CARD_OK)
    {
        BddTargetBoardDisk_Say(
            "read failed, sector %lu: %s",
            (unsigned long) sector,
            BddTargetBoardDisk_Reason(result)
        );
    }
    return result;
}

/* A transfer the card driver had to repeat succeeded in the end, and would
 * otherwise leave no sign that the wiring corrupted it. One line for the
 * transfer, with the number of repeats since the card was started. */
static void BddTargetBoardDisk_SayRepeats(const char* what, uint32_t sector)
{
    uint32_t repeats = BddTargetSdCard_Repeats();
    if (repeats != repeatsSaid)
    {
        BddTargetBoardDisk_Say(
            "%s repeated after a CRC failure, sector %lu, %lu so far",
            what,
            (unsigned long) sector,
            (unsigned long) repeats
        );
        repeatsSaid = repeats;
    }
}

/* A card that has lost its state - a wire that came out and went back, a dip
 * in its supply - answers nothing until it is started again. True when it
 * has been, so the sector can be tried once more. A card that will not start
 * leaves the disk not ready. */
static bool BddTargetBoardDisk_StartAgain(const char* what, uint32_t sector, enum BddTargetSdCardResult result)
{
    BddTargetBoardDisk_Say(
        "%s of sector %lu: %s, starting the card again",
        what,
        (unsigned long) sector,
        BddTargetBoardDisk_Reason(result)
    );
    ready = false;
    return BddTargetBoardDisk_EnsureReady();
}

enum BddTargetBoardDiskResult BddTargetBoardDisk_Write(const void* buffer, uint32_t sector, uint32_t count)
{
    enum BddTargetBoardDiskResult result = BDD_TARGET_BOARD_DISK_OK;
    if (!BddTargetBoardDisk_IsRangeValid(sector, count))
    {
        result = BDD_TARGET_BOARD_DISK_OUT_OF_RANGE;
    }
    else
    {
        const uint8_t* bytes = buffer;
        for (uint32_t i = 0U; (i < count) && (result == BDD_TARGET_BOARD_DISK_OK); i++)
        {
            const uint8_t* from = &bytes[(size_t) i * (size_t) BDD_TARGET_BOARD_DISK_SECTOR_SIZE];
            if (BddTargetBoardDisk_WriteSector(sector + i, from) != BDD_TARGET_SD_CARD_OK)
            {
                result = BDD_TARGET_BOARD_DISK_IO_ERROR;
            }
        }
    }
    return result;
}

/* Writing a sector twice is harmless, so the write is safe to repeat after
 * the card has been started again. */
static enum BddTargetSdCardResult BddTargetBoardDisk_WriteSector(uint32_t sector, const uint8_t* bytes)
{
    enum BddTargetSdCardResult result = BddTargetSdCard_Write(sector, bytes);
    BddTargetBoardDisk_SayRepeats("write", sector);
    if ((result != BDD_TARGET_SD_CARD_OK) && BddTargetBoardDisk_StartAgain("write", sector, result))
    {
        result = BddTargetSdCard_Write(sector, bytes);
        BddTargetBoardDisk_SayRepeats("write", sector);
    }
    if (result != BDD_TARGET_SD_CARD_OK)
    {
        BddTargetBoardDisk_Say(
            "write failed, sector %lu: %s",
            (unsigned long) sector,
            BddTargetBoardDisk_Reason(result)
        );
    }
    return result;
}

/* A filesystem is recognised by its first sector, so zeroing that sector
 * leaves none, and the next mount formats the disk over whatever the later
 * sectors held. */
bool BddTargetBoardDisk_Wipe(void)
{
    static const uint8_t ZEROS[BDD_TARGET_BOARD_DISK_SECTOR_SIZE] = {0};
    return BddTargetBoardDisk_EnsureReady() && (BddTargetBoardDisk_Write(ZEROS, 0U, 1U) == BDD_TARGET_BOARD_DISK_OK);
}
