/* A store kept directly on the board's NOR flash - see BddTargetNorFlashStore.h. */

#include "BddTargetNorFlashStore.h"

#include "BddTargetBoardNorFlashStoreRun.h"
#include "BddTargetBoardSpiNorFlashBus.h"
#include "BddTargetCountingNorFlash.h"
#include "BddTargetStoreSettings.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogSpiNorFlash.h"

#include <stddef.h>

enum
{
    /* What SolidSyslogNorFlashBlockDeviceConfig takes as a BlockSize to mean
     * the flash block less the device's own state. */
    ALL_A_FLASH_BLOCK_HOLDS = 0
};

/* NULL until the part is first asked to start, and again after Unmount. */
static struct SolidSyslogNorFlash* flash;
/* The same flash with its erases, programs and reads counted on the way,
 * which is the one the block device is given. */
static struct SolidSyslogNorFlash* countedFlash;

static inline bool NorFlashStore_IsStarted(void);
static struct SolidSyslogBlockDevice* NorFlashStore_CreateDevice(size_t blockSize);
static inline size_t NorFlashStore_PartBlockCount(void);

bool BddTargetNorFlashStore_Mount(void)
{
    if (!NorFlashStore_IsStarted())
    {
        const struct SolidSyslogSpiNorFlashConfig config = {BddTargetBoardSpiNorFlashBus_Take()};
        flash = SolidSyslogSpiNorFlash_Create(&config);
        countedFlash = BddTargetCountingNorFlash_Over(flash);
    }
    return NorFlashStore_IsStarted();
}

static inline bool NorFlashStore_IsStarted(void)
{
    return (flash != NULL) && (flash != SolidSyslogNullNorFlash_Get());
}

/* Only a part that started has anything to let go of: destroying what was
 * never created is reported. */
void BddTargetNorFlashStore_Unmount(void)
{
    if (NorFlashStore_IsStarted())
    {
        SolidSyslogSpiNorFlash_Destroy(flash);
    }
    flash = NULL;
    countedFlash = NULL;
    (void) BddTargetCountingNorFlash_Over(NULL);
}

struct SolidSyslogBlockDevice* BddTargetNorFlashStore_CreateBlockDevice(void)
{
    size_t blockSize = (size_t) ALL_A_FLASH_BLOCK_HOLDS;
    if (BddTargetStoreSettings_MaxBlockSizeIsSet())
    {
        blockSize = BddTargetStoreSettings_MaxBlockSize();
    }
    return NorFlashStore_CreateDevice(blockSize);
}

static struct SolidSyslogBlockDevice* NorFlashStore_CreateDevice(size_t blockSize)
{
    const struct BddTargetBoardNorFlashStoreRun run =
        BddTargetBoardNorFlashStoreRun_Within(NorFlashStore_PartBlockCount());
    const struct SolidSyslogNorFlashBlockDeviceConfig config = {
        .Flash = countedFlash,
        .FirstBlock = run.FirstBlock,
        .BlockCount = run.BlockCount,
        .BlockSize = blockSize,
    };
    return SolidSyslogNorFlashBlockDevice_Create(&config);
}

/* A part that has not started has no blocks for the board to state a run of.
 * The device's Create then refuses whatever run the board answers with. */
static inline size_t NorFlashStore_PartBlockCount(void)
{
    return NorFlashStore_IsStarted() ? SolidSyslogNorFlash_GetBlockCount(flash) : 0U;
}

void BddTargetNorFlashStore_DestroyBlockDevice(struct SolidSyslogBlockDevice* device)
{
    SolidSyslogNorFlashBlockDevice_Destroy(device);
}

/* Through a block device of its own, which it gives back, so the wipe knows
 * no more of how a block is kept than the store does. */
bool BddTargetNorFlashStore_Wipe(void)
{
    bool wiped = BddTargetNorFlashStore_Mount();
    if (wiped)
    {
        struct SolidSyslogBlockDevice* device = NorFlashStore_CreateDevice((size_t) ALL_A_FLASH_BLOCK_HOLDS);
        size_t blockCount = SolidSyslogBlockDevice_GetBlockCount(device);
        for (size_t block = 0U; wiped && (block < blockCount); block++)
        {
            if (SolidSyslogBlockDevice_Exists(device, block))
            {
                wiped = SolidSyslogBlockDevice_Dispose(device, block);
            }
        }
        SolidSyslogNorFlashBlockDevice_Destroy(device);
    }
    return wiped;
}
