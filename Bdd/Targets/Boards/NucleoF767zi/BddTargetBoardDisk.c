/* The NUCLEO-F767ZI's disk behind the board seam - see BddTargetBoardDisk.h.
 *
 * It is the first BDD_TARGET_BOARD_DISK_SECTOR_COUNT sectors of a MicroSD
 * card on SPI4, driven by BddTargetSdCard.c over the bus in
 * BddTargetBoardSdCardBus.c. With no card fitted the disk never becomes
 * ready, a mount over it fails, and the target stays on the store it started
 * with.
 *
 * The card is on a breadboard, at the end of wires that can work loose, so a
 * sector that will not transfer has the card started again and is tried once
 * more before the disk gives up on it.
 *
 * What goes wrong is said on the console, so that a card that has stopped
 * answering is told from a store that has a fault of its own. */

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
/* What EnsureReady last said, so that a mount that is tried again and again
 * over a card that is not there says so once. */
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

/* One line, whole, in one write, which the console lock keeps from mixing
 * with what another task is saying. */
static void BddTargetBoardDisk_Say(const char* format, ...)
{
    char line[LINE_BYTES];
    size_t length = sizeof(LINE_START) - 1U;
    va_list arguments;
    (void) snprintf(line, sizeof(line), "%s", LINE_START);
    va_start(arguments, format);
    int formatted = vsnprintf(&line[length], sizeof(line) - length, format, arguments);
    va_end(arguments);
    /* Room is kept for the newline: a line too long for it is not said. */
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

/* Range-checks a span of sectors without overflow: the subtraction keeps
 * `sector + count` from wrapping past the comparison. */
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

/* A transfer the card driver had to repeat worked in the end, and would
 * otherwise leave no trace of the wires having damaged it. One line for the
 * transfer, with how many repeats there have been since the card was started. */
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

/* A card that has lost its place - a wire that came out and went back, a dip
 * in its supply - answers nothing it is asked until it is started again.
 * True when it has been, and the sector is worth trying once more. A card
 * that will not start leaves the disk not ready. */
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

/* A sector written twice holds what it was sent, so writing one again after
 * the card has been started again is safe. */
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

/* A filesystem is found by what its first sector holds, so a disk whose first
 * sector is zeros has none, and the mount that finds none formats it. What
 * the sectors after it held is written over by that format. */
bool BddTargetBoardDisk_Wipe(void)
{
    static const uint8_t ZEROS[BDD_TARGET_BOARD_DISK_SECTOR_SIZE] = {0};
    return BddTargetBoardDisk_EnsureReady() && (BddTargetBoardDisk_Write(ZEROS, 0U, 1U) == BDD_TARGET_BOARD_DISK_OK);
}
