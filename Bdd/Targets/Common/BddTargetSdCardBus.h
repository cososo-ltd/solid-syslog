#ifndef BDD_TARGET_SD_CARD_BUS_H
#define BDD_TARGET_SD_CARD_BUS_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The SPI bus an SD card sits on, as the card driver (BddTargetSdCard.h)
     * needs it. Part of the board seam - see BddTargetBoard.h - and defined in
     * the directory of a board that has a card.
     *
     * SPI mode 0, most significant bit first. The card's select is the
     * board's to drive, because a card is started with it high. */

    /* Takes the bus, at a clock of no more than 400 kHz, which is what a card
     * is to be started at. Idempotent. */
    void BddTargetSdCardBus_UseStartRate(void);

    /* Clocks the bus at the rate the board runs a started card at. */
    void BddTargetSdCardBus_UseRunRate(void);

    void BddTargetSdCardBus_Select(void);
    void BddTargetSdCardBus_Deselect(void);

    /* Clocks one byte out and returns the byte clocked in with it. A bus with
     * no card on it reads 0xFF. */
    uint8_t BddTargetSdCardBus_Exchange(uint8_t out);

    /* A count of milliseconds, wrapping at 2^32, to time the card's answers by. */
    uint32_t BddTargetSdCardBus_Milliseconds(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_SD_CARD_BUS_H */
