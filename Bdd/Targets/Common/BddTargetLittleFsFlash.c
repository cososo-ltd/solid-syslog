/* LittleFS block device over an SPI NOR flash - see BddTargetLittleFsDisk.h.
 *
 * The flash is the SpiNorFlash pack's, over the bus the board supplies
 * (BddTargetBoardSpiNorFlashBus.h): the same flash code a store kept
 * directly on the part runs over. A filesystem block is one block of the
 * flash, which is one of the part's smallest erases.
 *
 * The filesystem uses the first BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT blocks
 * of the part, whatever the part's size.
 *
 * The pack reports why a part did not start, or why an operation failed,
 * through the library's error handler. What is said here, on the console,
 * is which operation of the filesystem's met it. LittleFS calls these
 * functions from whichever thread is using the store, so nothing here uses
 * stdio, which belongs to the console thread. */

#include "BddTargetLittleFsDisk.h"

#include "BddTargetBoardSpiNorFlashBus.h"
#include "BddTargetCountingNorFlash.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogSpiNorFlash.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

enum
{
    /* The part reads any length from any address. */
    READ_SIZE = 1,
    /* The part programs any part of a page. LittleFS ends every commit on a
     * multiple of this, so a small value lets a metadata block take many
     * commits before it has to be compacted and erased. */
    PROG_SIZE = 16,
    FIRST_SUPERBLOCK = 0,
    SECOND_SUPERBLOCK = 1,
    LINE_BYTES = 128
};

static const char LINE_START[] = "[board] flash: ";

static uint8_t readBuffer[BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE];
static uint8_t progBuffer[BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE];
static uint8_t lookaheadBuffer[BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE];

/* NULL until the part first starts, and again after a part that started and
 * did not fit the filesystem. */
static struct SolidSyslogNorFlash* flash;
/* The same flash with its erases, programs and reads counted on the way,
 * which is the one the filesystem's operations go to. */
static struct SolidSyslogNorFlash* countedFlash;
/* So a mount retried over a part that will not start, or does not fit, says
 * so once. */
static bool startFailureSaid;
static bool misfitSaid;

static int FlashRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int FlashProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int FlashErase(const struct lfs_config* c, lfs_block_t block);
static int FlashSync(const struct lfs_config* c);

static bool LittleFsFlash_EnsureReady(void);
static inline bool LittleFsFlash_IsStarted(void);
static void LittleFsFlash_Start(void);
static inline bool LittleFsFlash_FitsTheFilesystem(void);
static void LittleFsFlash_Say(const char* format, ...) __attribute__((format(printf, 1, 2)));
static int LittleFsFlash_Judge(const char* what, lfs_block_t block, bool succeeded);

static const struct lfs_config CONFIG = {
    .context = NULL,
    .read = FlashRead,
    .prog = FlashProg,
    .erase = FlashErase,
    .sync = FlashSync,
    .read_size = READ_SIZE,
    .prog_size = PROG_SIZE,
    .block_size = BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE,
    .block_count = BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT,
    .block_cycles = 500,
    .cache_size = BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE,
    .lookahead_size = BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE,
    .read_buffer = readBuffer,
    .prog_buffer = progBuffer,
    .lookahead_buffer = lookaheadBuffer,
};

const struct lfs_config* BddTargetLittleFsDisk_Config(void)
{
    return &CONFIG;
}

/* LittleFS finds a filesystem by its pair of superblocks, so with both
 * erased the next mount finds none and formats. The blocks after them are
 * erased as each is next used. */
bool BddTargetLittleFsDisk_Wipe(void)
{
    return LittleFsFlash_EnsureReady() && (FlashErase(&CONFIG, FIRST_SUPERBLOCK) == LFS_ERR_OK) &&
           (FlashErase(&CONFIG, SECOND_SUPERBLOCK) == LFS_ERR_OK);
}

/* A part that did not start is tried again at the next operation: creating
 * the flash waits for whatever the part was left doing. */
static bool LittleFsFlash_EnsureReady(void)
{
    if (!LittleFsFlash_IsStarted())
    {
        LittleFsFlash_Start();
    }
    return LittleFsFlash_IsStarted();
}

static inline bool LittleFsFlash_IsStarted(void)
{
    return (flash != NULL) && (flash != SolidSyslogNullNorFlash_Get());
}

static void LittleFsFlash_Start(void)
{
    const struct SolidSyslogSpiNorFlashConfig config = {BddTargetBoardSpiNorFlashBus_Take()};
    flash = SolidSyslogSpiNorFlash_Create(&config);
    countedFlash = BddTargetCountingNorFlash_Over(flash);

    if (!LittleFsFlash_IsStarted())
    {
        if (!startFailureSaid)
        {
            LittleFsFlash_Say("the part did not start");
            startFailureSaid = true;
        }
    }
    else if (!LittleFsFlash_FitsTheFilesystem())
    {
        if (!misfitSaid)
        {
            LittleFsFlash_Say(
                "the part has %lu blocks of %lu bytes, and the filesystem needs %lu of %lu",
                (unsigned long) SolidSyslogNorFlash_GetBlockCount(flash),
                (unsigned long) SolidSyslogNorFlash_GetBlockSize(flash),
                (unsigned long) BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT,
                (unsigned long) BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE
            );
            misfitSaid = true;
        }
        SolidSyslogSpiNorFlash_Destroy(flash);
        flash = NULL;
        (void) BddTargetCountingNorFlash_Over(NULL);
    }
    else
    {
        /* Started, and it fits. A later failure to start is news again. */
        startFailureSaid = false;
    }
}

/* A filesystem block is one block of the flash, so the part's smallest erase
 * must be the filesystem's block size. */
static inline bool LittleFsFlash_FitsTheFilesystem(void)
{
    return (SolidSyslogNorFlash_GetBlockSize(flash) == (size_t) BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE) &&
           (SolidSyslogNorFlash_GetBlockCount(flash) >= (size_t) BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT);
}

/* One whole line in one write, which the console lock keeps from
 * interleaving with another thread's output. */
static void LittleFsFlash_Say(const char* format, ...)
{
    char line[LINE_BYTES];
    size_t length = sizeof(LINE_START) - 1U;
    va_list arguments;
    (void) snprintf(line, sizeof(line), "%s", LINE_START);
    va_start(arguments, format);
    int formatted = vsnprintf(&line[length], sizeof(line) - length, format, arguments);
    va_end(arguments);
    /* Leaves room for the newline; a line too long for it is dropped. */
    if ((formatted > 0) && (((size_t) formatted + length) < (sizeof(line) - 1U)))
    {
        length += (size_t) formatted;
        line[length] = '\n';
        (void) write(STDOUT_FILENO, line, length + 1U);
    }
}

static int FlashRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    (void) c;
    int result = LFS_ERR_IO;
    if (LittleFsFlash_EnsureReady())
    {
        result = LittleFsFlash_Judge("read", block, SolidSyslogNorFlash_Read(countedFlash, block, off, buffer, size));
    }
    return result;
}

/* The flash stays created after a failure. A part left busy fails every
 * operation at once until it is ready again, and then carries on. */
static int LittleFsFlash_Judge(const char* what, lfs_block_t block, bool succeeded)
{
    if (!succeeded)
    {
        LittleFsFlash_Say("%s failed, block %lu", what, (unsigned long) block);
    }
    return succeeded ? LFS_ERR_OK : LFS_ERR_IO;
}

static int FlashProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    (void) c;
    int result = LFS_ERR_IO;
    if (LittleFsFlash_EnsureReady())
    {
        result =
            LittleFsFlash_Judge("program", block, SolidSyslogNorFlash_Program(countedFlash, block, off, buffer, size));
    }
    return result;
}

static int FlashErase(const struct lfs_config* c, lfs_block_t block)
{
    (void) c;
    int result = LFS_ERR_IO;
    if (LittleFsFlash_EnsureReady())
    {
        result = LittleFsFlash_Judge("erase", block, SolidSyslogNorFlash_Erase(countedFlash, block));
    }
    return result;
}

/* The flash returns from a program or an erase only once the part has
 * finished, so there is nothing to flush. */
static int FlashSync(const struct lfs_config* c)
{
    (void) c;
    return LFS_ERR_OK;
}
