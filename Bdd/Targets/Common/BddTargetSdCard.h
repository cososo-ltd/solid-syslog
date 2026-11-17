#ifndef BDD_TARGET_SD_CARD_H
#define BDD_TARGET_SD_CARD_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* An SD card in SPI mode, as sectors of 512 bytes, over the bus the board
     * supplies (BddTargetBoardSdCardBus.h). One card.
     *
     * It follows the SD Physical Layer Simplified Specification and supports
     * only a version 2 card, standard or high capacity, read and written one
     * sector at a time. A version 1 card and an MMC are refused.
     *
     * The card's CRC checking is turned on, so a command or a sector
     * corrupted on the bus is rejected by the end that receives it. A read or
     * a write that fails that way is retried twice. */

    enum
    {
        BDD_TARGET_SD_CARD_SECTOR_SIZE = 512
    };

    enum BddTargetSdCardResult
    {
        BDD_TARGET_SD_CARD_OK = 0,
        /* Read or Write called before a successful Start. */
        BDD_TARGET_SD_CARD_NOT_STARTED,
        /* Nothing answered a command: no card, or no path to it. */
        BDD_TARGET_SD_CARD_NO_ANSWER,
        /* A card answered, but not one this driver supports. */
        BDD_TARGET_SD_CARD_UNSUPPORTED,
        /* The card answered a command with an error, or sent an error in
         * place of the data it was asked for. */
        BDD_TARGET_SD_CARD_REFUSED,
        /* The card stayed busy, or never sent the data it was asked for. */
        BDD_TARGET_SD_CARD_TIMED_OUT,
        /* A command or a sector failed its CRC on every attempt. */
        BDD_TARGET_SD_CARD_CRC_FAILED,
        /* The card could not store a sector it was sent, or says the write failed. */
        BDD_TARGET_SD_CARD_WRITE_FAILED
    };

    /* Brings the card from power-up to ready. It can be called again after a
     * failure, or to restart a card that has been changed. */
    enum BddTargetSdCardResult BddTargetSdCard_Start(void);

    /* Forgets the card, so nothing is read or written until it is started
     * again. */
    void BddTargetSdCard_Stop(void);

    /* True from a successful Start until Stop or a failed Start. */
    bool BddTargetSdCard_IsStarted(void);

    /* The card's capacity in sectors, as it reports it. Zero unless started. */
    uint32_t BddTargetSdCard_SectorCount(void);

    /* The number of retries after a CRC failure since the card was started:
     * bus errors that the retries hid. */
    uint32_t BddTargetSdCard_Repeats(void);

    /* `buffer` is BDD_TARGET_SD_CARD_SECTOR_SIZE bytes. */
    enum BddTargetSdCardResult BddTargetSdCard_Read(uint32_t sector, uint8_t* buffer);
    enum BddTargetSdCardResult BddTargetSdCard_Write(uint32_t sector, const uint8_t* buffer);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_SD_CARD_H */
