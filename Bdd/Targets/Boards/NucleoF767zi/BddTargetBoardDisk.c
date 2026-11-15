/* The NUCLEO-F767ZI's disk behind the board seam - see BddTargetBoardDisk.h.
 *
 * It is the first BDD_TARGET_BOARD_DISK_SECTOR_COUNT sectors of a MicroSD
 * card on SPI4, driven by BddTargetSdCard.c over the bus in
 * BddTargetBoardSdCardBus.c. With no card fitted the disk never becomes ready, a
 * mount over it fails, and the target stays on the store it started with.
 *
 * What goes wrong is said on the console, once for each thing that goes
 * wrong, so that a card that has stopped answering is told from a store that
 * has a fault of its own. */

#include "BddTargetBoardDisk.h"

#include "BddTargetSdCard.h"

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

enum
{
    REPORT_BYTES = 112
};

static bool ready;
static enum BddTargetSdCardResult lastStartReported = BDD_TARGET_SD_CARD_OK;
static uint32_t repeatsReported;

static inline const char* BddTargetBoardDisk_Reason(enum BddTargetSdCardResult result);
static void BddTargetBoardDisk_Report(const char* what, enum BddTargetSdCardResult result);
static void BddTargetBoardDisk_ReportSector(const char* what, uint32_t sector, enum BddTargetSdCardResult result);
static void BddTargetBoardDisk_ReportRepeats(const char* what, uint32_t sector);
static bool BddTargetBoardDisk_IsRangeValid(uint32_t sector, uint32_t count);

bool BddTargetBoardDisk_EnsureReady(void)
{
    if (!ready)
    {
        enum BddTargetSdCardResult result = BddTargetSdCard_Start();
        if (result != BDD_TARGET_SD_CARD_OK)
        {
            /* A mount that fails is tried again, and a card that is not there
             * would otherwise say so every time. */
            if (result != lastStartReported)
            {
                BddTargetBoardDisk_Report("the card did not start", result);
            }
        }
        else if (BddTargetSdCard_SectorCount() < (uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT)
        {
            BddTargetBoardDisk_Report("the card is smaller than the disk", result);
            BddTargetSdCard_Stop();
        }
        else
        {
            ready = true;
        }
        lastStartReported = result;
    }
    return ready;
}

static void BddTargetBoardDisk_Report(const char* what, enum BddTargetSdCardResult result)
{
    char line[REPORT_BYTES];
    int formatted = snprintf(line, sizeof(line), "[board] disk: %s: %s\n", what, BddTargetBoardDisk_Reason(result));
    if ((formatted > 0) && ((size_t) formatted < sizeof(line)))
    {
        (void) write(STDOUT_FILENO, line, (size_t) formatted);
    }
}

static void BddTargetBoardDisk_ReportSector(const char* what, uint32_t sector, enum BddTargetSdCardResult result)
{
    char line[REPORT_BYTES];
    int formatted = snprintf(
        line,
        sizeof(line),
        "[board] disk: %s, sector %lu: %s\n",
        what,
        (unsigned long) sector,
        BddTargetBoardDisk_Reason(result)
    );
    if ((formatted > 0) && ((size_t) formatted < sizeof(line)))
    {
        (void) write(STDOUT_FILENO, line, (size_t) formatted);
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
            enum BddTargetSdCardResult read =
                BddTargetSdCard_Read(sector + i, &bytes[(size_t) i * (size_t) BDD_TARGET_BOARD_DISK_SECTOR_SIZE]);
            BddTargetBoardDisk_ReportRepeats("read", sector + i);
            if (read != BDD_TARGET_SD_CARD_OK)
            {
                BddTargetBoardDisk_ReportSector("read failed", sector + i, read);
                result = BDD_TARGET_BOARD_DISK_IO_ERROR;
            }
        }
    }
    return result;
}

/* A transfer the card driver had to repeat worked in the end, and would
 * otherwise leave no trace of the wires having damaged it. */
static void BddTargetBoardDisk_ReportRepeats(const char* what, uint32_t sector)
{
    uint32_t repeats = BddTargetSdCard_Repeats();
    if (repeats != repeatsReported)
    {
        char line[REPORT_BYTES];
        int formatted = snprintf(
            line,
            sizeof(line),
            "[board] disk: %s repeated after a CRC failure, sector %lu, %lu so far\n",
            what,
            (unsigned long) sector,
            (unsigned long) repeats
        );
        if ((formatted > 0) && ((size_t) formatted < sizeof(line)))
        {
            (void) write(STDOUT_FILENO, line, (size_t) formatted);
        }
        repeatsReported = repeats;
    }
}

/* Range-checks a span of sectors without overflow: the subtraction keeps
 * `sector + count` from wrapping past the comparison. */
static bool BddTargetBoardDisk_IsRangeValid(uint32_t sector, uint32_t count)
{
    return (count <= (uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT) &&
           (sector <= ((uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT - count));
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
            enum BddTargetSdCardResult written =
                BddTargetSdCard_Write(sector + i, &bytes[(size_t) i * (size_t) BDD_TARGET_BOARD_DISK_SECTOR_SIZE]);
            BddTargetBoardDisk_ReportRepeats("write", sector + i);
            if (written != BDD_TARGET_SD_CARD_OK)
            {
                BddTargetBoardDisk_ReportSector("write failed", sector + i, written);
                result = BDD_TARGET_BOARD_DISK_IO_ERROR;
            }
        }
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
