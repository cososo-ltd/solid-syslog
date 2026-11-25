#ifndef BDD_TARGET_NOR_FLASH_STORE_H
#define BDD_TARGET_NOR_FLASH_STORE_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

struct SolidSyslogBlockDevice;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* A store kept directly on the board's NOR flash, with no filesystem: the
     * library's SpiNorFlash pack over the bus the board supplies
     * (BddTargetBoardSpiNorFlashBus.h), on the run of the part's blocks the
     * board states (BddTargetBoardNorFlashStoreRun.h). Fills the store half
     * of the shared pipeline's seam - see BddTargetFreeRtosPipeline.h. */

    /* Starts the part. Answers true once it has started, and again at once on
     * every call after that. A part that did not start is tried again at the
     * next call. */
    bool BddTargetNorFlashStore_Mount(void);

    /* Lets go of the part, so that the next Mount starts it again. Safe to
     * call with no part started. There is nothing to flush: a program has
     * finished, and been read back, by the time it returns. */
    void BddTargetNorFlashStore_Unmount(void);

    /* The store's block device, over the board's run of the part Mount
     * started. The board is asked for the run each time, and told how many
     * blocks the part has. A block is all a flash block holds, unless the
     * harness has set a max-block-size, which is then passed as it stands: one
     * more than a flash block holds is refused, and the answer is the Null
     * block device. So it is for a run the part does not have, and where no
     * part has started. */
    struct SolidSyslogBlockDevice* BddTargetNorFlashStore_CreateBlockDevice(void);
    void BddTargetNorFlashStore_DestroyBlockDevice(struct SolidSyslogBlockDevice * device);

    /* Leaves the store's run holding no block, so the next store starts
     * empty. For a part that keeps its contents from one run of the target
     * to the next; call with no block device over the part. It starts the
     * part if need be, and disposes of each block that exists. Nothing is
     * erased, because the store erases every block as it takes it. Answers
     * false if the part does not start, if it does not have the run the
     * board states, or if a block cannot be disposed of. */
    bool BddTargetNorFlashStore_Wipe(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_NOR_FLASH_STORE_H */
