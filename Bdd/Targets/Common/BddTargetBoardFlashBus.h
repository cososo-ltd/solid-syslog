#ifndef BDD_TARGET_BOARD_FLASH_BUS_H
#define BDD_TARGET_BOARD_FLASH_BUS_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The SPI bus a NOR flash sits on, as the flash driver
     * (BddTargetSpiNorFlash.h) needs it. Part of the board seam - see
     * BddTargetBoard.h - and defined in the directory of a board that has a
     * flash.
     *
     * SPI mode 0, most significant bit first. The board drives the part's
     * select: a command begins when it falls, and a program or an erase
     * starts when it rises. */

    /* Takes the bus, at the rate the board runs the part at. Idempotent. */
    void BddTargetBoardFlashBus_Take(void);

    void BddTargetBoardFlashBus_Select(void);
    void BddTargetBoardFlashBus_Deselect(void);

    /* Clocks one byte out and returns the byte clocked in with it. A bus with
     * no part on it reads 0xFF. */
    uint8_t BddTargetBoardFlashBus_Exchange(uint8_t out);

    /* Milliseconds, wrapping at 2^32, for timing the part. */
    uint32_t BddTargetBoardFlashBus_Milliseconds(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_FLASH_BUS_H */
