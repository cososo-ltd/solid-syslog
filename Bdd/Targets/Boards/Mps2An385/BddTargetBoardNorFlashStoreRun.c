/* The run of the QEMU mps2-an385 machine's simulated flash that a store with
 * no filesystem may use - see BddTargetBoardNorFlashStoreRun.h. The last 2048
 * of the part's 4 KiB sectors, which is 8 MiB, so the run ends where the part
 * does. On the simulated W25Q128JV that is sectors 2048 to 4095. */

#include "BddTargetBoardNorFlashStoreRun.h"

enum
{
    BLOCK_COUNT = 2048
};

/* Counted back from the end of the part. Where the part has fewer blocks than
 * the run, the subtraction wraps and the first block it gives is never used:
 * the store's block device is refused on the count before the first block is
 * looked at. */
struct BddTargetBoardNorFlashStoreRun BddTargetBoardNorFlashStoreRun_Within(size_t flashBlockCount)
{
    const struct BddTargetBoardNorFlashStoreRun run = {flashBlockCount - (size_t) BLOCK_COUNT, BLOCK_COUNT};
    return run;
}
