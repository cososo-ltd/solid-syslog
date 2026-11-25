/* The run of the NUCLEO-F767ZI's W25Q128 that a store with no filesystem may
 * use - see BddTargetBoardNorFlashStoreRun.h. 2048 of the part's 4 KiB
 * sectors from sector 1024, which is 8 MiB of its 16 with 4 MiB either side:
 * sectors 1024 to 3071.
 *
 * A run in the middle of the part, as a product that fits a store to a board
 * it already has may have to give it. It is clear of the part's last sector,
 * which the bring-up image's flash check erases. */

#include "BddTargetBoardNorFlashStoreRun.h"

enum
{
    FIRST_BLOCK = 1024,
    BLOCK_COUNT = 2048
};

/* The same run whatever the part has. One that is not all within the part is
 * refused where the store's block device is created. */
struct BddTargetBoardNorFlashStoreRun BddTargetBoardNorFlashStoreRun_Within(size_t flashBlockCount)
{
    const struct BddTargetBoardNorFlashStoreRun run = {FIRST_BLOCK, BLOCK_COUNT};
    (void) flashBlockCount;
    return run;
}
