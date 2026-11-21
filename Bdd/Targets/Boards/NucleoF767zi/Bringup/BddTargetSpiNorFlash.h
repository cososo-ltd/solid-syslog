#ifndef BDD_TARGET_SPI_NOR_FLASH_H
#define BDD_TARGET_SPI_NOR_FLASH_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* An SPI NOR flash, as bytes that are read, programmed and erased, over the
     * bus the board supplies (BddTargetBoardFlashBus.h). One part. It is the
     * bring-up image's own driver, with no SolidSyslog in it, so that a fault
     * in the board or the part can be told from one in the library's flash
     * pack, which the BDD targets use.
     *
     * It supports only a Winbond W25Q of up to 16 MiB, the most a three-byte
     * address reaches, using the plain SPI commands in the part's datasheet.
     * Any other part is refused, including Winbond's other families. */

    enum
    {
        /* The most a single program command takes. A longer program is split
         * at page boundaries. */
        BDD_TARGET_SPI_NOR_FLASH_PAGE_SIZE = 256,
        /* The smallest unit that can be erased. */
        BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE = 4096
    };

    enum BddTargetSpiNorFlashResult
    {
        BDD_TARGET_SPI_NOR_FLASH_OK = 0,
        /* Called before a successful Start. */
        BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED,
        /* Part of the range is past the end of the part. */
        BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE,
        /* Nothing answered: no part, or no path to it. */
        BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER,
        /* A part answered, but not one this driver supports. */
        BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED,
        /* The part stayed busy. After a program or an erase it is no longer
         * started, and must be started again. */
        BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT,
        /* The part would not enable writing, so nothing was programmed or erased. */
        BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED
    };

    /* A short text for a result, for a console line. */
    const char* BddTargetSpiNorFlash_ResultText(enum BddTargetSpiNorFlashResult result);

    /* Waits for the part to finish anything a reset interrupted, then
     * identifies it. It can be called again after a failure. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Start(void);

    /* Forgets the part, so nothing is read, programmed or erased until it is
     * started again. */
    void BddTargetSpiNorFlash_Stop(void);

    /* True from a successful Start until Stop, a failed Start, or a program
     * or an erase that times out. */
    bool BddTargetSpiNorFlash_IsStarted(void);

    /* The part's size in bytes, as it identifies itself. Zero unless started. */
    uint32_t BddTargetSpiNorFlash_Capacity(void);

    /* Reads `count` bytes starting at `address`, anywhere in the part. The
     * part sends no checksum, so data corrupted on the bus goes undetected.
     * A part that has stopped answering is detected: its status is read
     * after the data. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Read(uint32_t address, uint8_t* buffer, size_t count);

    /* Programs `count` bytes starting at `address`, anywhere in the part,
     * and returns once the part has finished. A program only clears bits:
     * each byte becomes its old value ANDed with the new one, so only erased
     * bytes take the value given. The part does not report whether they
     * did. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_Program(uint32_t address, const uint8_t* bytes, size_t count);

    /* Sets every byte of a sector to 0xFF, and returns once the part has
     * finished. Sector `n` starts at `n` times
     * BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE. */
    enum BddTargetSpiNorFlashResult BddTargetSpiNorFlash_EraseSector(uint32_t sector);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_SPI_NOR_FLASH_H */
