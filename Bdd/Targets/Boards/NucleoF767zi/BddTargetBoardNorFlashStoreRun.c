/* The run of the NUCLEO-F767ZI's W25Q128 that a store with no filesystem may
 * use - see BddTargetBoardNorFlashStoreRun.h. 1024 of the part's 4 KiB
 * sectors from sector 2048, which is 4 MiB of its 16: sectors 2048 to 3071.
 *
 * A run that is neither the start of the part nor its end, as a product that
 * fits a store to a board it already has may have to give it. It begins where
 * the LittleFS image's filesystem ends, so neither image writes in the
 * other's sectors, and it is clear of the part's last sector, which the
 * bring-up image's flash check erases. */

#include "BddTargetBoardNorFlashStoreRun.h"

enum
{
    FIRST_BLOCK = 2048,
    BLOCK_COUNT = 1024
};

/* The same run whatever the part has. One that is not all within the part is
 * refused where the store's block device is created. */
struct BddTargetBoardNorFlashStoreRun BddTargetBoardNorFlashStoreRun_Within(size_t flashBlockCount)
{
    const struct BddTargetBoardNorFlashStoreRun run = {FIRST_BLOCK, BLOCK_COUNT};
    (void) flashBlockCount;
    return run;
}
