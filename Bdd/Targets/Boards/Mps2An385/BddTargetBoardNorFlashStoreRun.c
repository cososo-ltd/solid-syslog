/* The run of the QEMU mps2-an385 machine's simulated flash that a store with
 * no filesystem may use - see BddTargetBoardNorFlashStoreRun.h. The first 2048
 * of the part's 4 KiB sectors, which is 8 MiB of its 16. */

#include "BddTargetBoardNorFlashStoreRun.h"

enum
{
    FIRST_BLOCK = 0,
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
