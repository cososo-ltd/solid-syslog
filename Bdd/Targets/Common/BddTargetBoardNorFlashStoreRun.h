#ifndef BDD_TARGET_BOARD_NOR_FLASH_STORE_RUN_H
#define BDD_TARGET_BOARD_NOR_FLASH_STORE_RUN_H

#include "SolidSyslogExternC.h"

#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Which of the blocks of the board's NOR flash a store kept on it with no
     * filesystem may use. Part of the board seam - see BddTargetBoard.h - and
     * defined in the directory of a board that has a flash, or simulates one.
     * The rest of the part is the board's, for whatever else it keeps there. */
    struct BddTargetBoardNorFlashStoreRun
    {
        size_t FirstBlock;
        size_t BlockCount;
    };

    /* The store's run in a part of that many blocks, which is what the part
     * itself reported. A board whose run is counted back from the end of its
     * part works it out from that. Asked of a part that did not start, the
     * count is 0, and the answer is not used. */
    struct BddTargetBoardNorFlashStoreRun BddTargetBoardNorFlashStoreRun_Within(size_t flashBlockCount);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_NOR_FLASH_STORE_RUN_H */
