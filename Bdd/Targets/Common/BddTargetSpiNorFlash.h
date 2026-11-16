#ifndef BDD_TARGET_SPI_NOR_FLASH_H
#define BDD_TARGET_SPI_NOR_FLASH_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* A SPI NOR flash, as bytes that are read, programmed and erased, over the
     * bus the board supplies (BddTargetBoardFlashBus.h). One part.
     *
     * It is narrow on purpose: a Winbond W25Q of no more than 16 MiB, which
     * three bytes of address reach the end of, driven with the commands the
     * part's datasheet gives for plain SPI. Any other part is refused, the
     * maker's other families among them. */

    enum
    {
        /* The most a single program command takes, and the unit a program is
         * split at. */
        BDD_TARGET_SPI_NOR_FLASH_PAGE_SIZE = 256,
        /* The least that can be erased. */
        BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE = 4096
    };

    enum BddTargetSpiNorFlashResult
    {
        BDD_TARGET_SPI_NOR_FLASH_OK = 0,
        /* Asked for before a Start that succeeded. */
        BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED,
        /* Some of what was asked for is past the end of the part. */
        BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE,
        /* Nothing answered: no part, or no path to it. */
        BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER,
        /* A part answered that is not one this drives. */
        BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED,
        /* The part stayed busy. After a program or an erase it is no longer
         * started, and is to be started again. */
        BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT,
        /* The part would not enable writing, so nothing was programmed or erased. */
        BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED
    };

    /* A few words for a result, for a line on a console. */
    const char* BddTargetSpiNorFlash_ResultText(enum BddTargetSpiNorFlashResult result);

    /* Waits for the part to finish whatever a reset interrupted, and
     * identifies it. It can be called again after it has failed. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Start(void);

    /* Forgets the part, so that nothing is read, programmed or erased until it
     * is started again. */
    void BddTargetSpiNorFlash_Stop(void);

    /* True from a Start that succeeded until Stop, a Start that fails, or a
     * program or an erase that times out. */
    bool BddTargetSpiNorFlash_IsStarted(void);

    /* The part's size in bytes, as it identifies itself. Zero unless started. */
    uint32_t BddTargetSpiNorFlash_Capacity(void);

    /* Reads `count` bytes from `address` on, which may be any bytes of the
     * part. The part sends no check with them, so bytes damaged on the wires
     * are not known to be. A part that has stopped answering is: it is asked
     * for its status once the bytes are in. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Read(uint32_t address, uint8_t* buffer, size_t count);

    /* Programs `count` bytes from `address` on, which may be any bytes of the
     * part, and returns once the part has finished. A program clears bits and
     * sets none: a byte ends as what it held ANDed with what it was sent, so
     * only bytes erased since they were last programmed take what they are
     * given. The part does not say whether they did. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Program(uint32_t address, const uint8_t* bytes, size_t count);

    /* Sets every byte of a sector to 0xFF, and returns once the part has
     * finished. Sector `n` is the BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE bytes
     * from `n` times that size on. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_EraseSector(uint32_t sector);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_SPI_NOR_FLASH_H */
