/* A store kept directly on the board's NOR flash - see BddTargetNorFlashStore.h. */

#include "BddTargetNorFlashStore.h"

#include "BddTargetBoardSpiNorFlashBus.h"
#include "BddTargetCountingNorFlash.h"
#include "BddTargetStoreSettings.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogSpiNorFlash.h"

#include <stddef.h>

enum
{
    FIRST_BLOCK = 0,
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
    const struct SolidSyslogNorFlashBlockDeviceConfig config = {
        .Flash = countedFlash,
        .FirstBlock = FIRST_BLOCK,
        .BlockCount = BDD_TARGET_NOR_FLASH_STORE_BLOCK_COUNT,
        .BlockSize = blockSize,
    };
    return SolidSyslogNorFlashBlockDevice_Create(&config);
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
