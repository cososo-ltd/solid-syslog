#ifndef BDD_TARGET_BOARD_SD_CARD_BUS_H
#define BDD_TARGET_BOARD_SD_CARD_BUS_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The SPI bus an SD card sits on, as the card driver (BddTargetSdCard.h)
     * needs it. Part of the board seam - see BddTargetBoard.h - and defined in
     * the directory of a board that has a card.
     *
     * SPI mode 0, most significant bit first. The board drives the card's
     * select, because a card is started with it high. */

    /* Takes the bus at no more than 400 kHz, the rate a card must be started
     * at. Idempotent. */
    void BddTargetBoardSdCardBus_UseStartRate(void);

    /* Clocks the bus at the rate the board runs a started card at. */
    void BddTargetBoardSdCardBus_UseRunRate(void);

    void BddTargetBoardSdCardBus_Select(void);
    void BddTargetBoardSdCardBus_Deselect(void);

    /* Clocks one byte out and returns the byte clocked in with it. A bus with
     * no card on it reads 0xFF. */
    uint8_t BddTargetBoardSdCardBus_Exchange(uint8_t out);

    /* Milliseconds, wrapping at 2^32, for timing the card's answers. */
    uint32_t BddTargetBoardSdCardBus_Milliseconds(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_SD_CARD_BUS_H */
