#ifndef SIMULATED_SPI_NOR_FLASH_H
#define SIMULATED_SPI_NOR_FLASH_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogSpiNorFlashBus;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* An SPI NOR flash part in software, and the bus it is on, for a machine
     * that has neither: a Winbond W25Q128JV, 16 MiB, behind the bus seam of
     * the library's SpiNorFlash pack. The pack's own driver runs over it, SFDP
     * and all, as it does over the part on a board.
     *
     * It behaves as the part does where a driver or a store could tell the
     * difference. It answers the SFDP table read from a W25Q128JV. A program
     * clears bits and sets none, and one that runs past the end of its page
     * goes round to the page's start. An erase sets a 4 KiB sector to ones.
     * Neither happens unless writing was enabled just before it. The part is
     * then busy for a time, in which it answers nothing but a request for its
     * status, and it disables writing as it finishes.
     *
     * It takes the six commands the pack sends: page program, read data, read
     * status, write enable, sector erase and read SFDP. Any other is not
     * answered. */

    /* How long the part is busy after each begins, by the count of
     * milliseconds it was given: about what a W25Q128JV takes for a page and
     * for a sector. */
    enum
    {
        SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS = 1,
        SIMULATED_SPI_NOR_FLASH_ERASE_MILLISECONDS = 50
    };

    /* What the part keeps its bytes in, which is the caller's: memory, or a
     * file. It holds the complement of what the part reads as, so one that
     * has never been written and holds zeros is an erased part. Every span it
     * is asked for is within the part. */
    struct SimulatedSpiNorFlashMedium
    {
        /* False if it could not give them all. */
        bool (*Read)(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, void* buffer, size_t count);
        /* False if it could not keep them all. */
        bool (*Write)(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, const void* buffer, size_t count);
    };

    struct SimulatedSpiNorFlashConfig
    {
        struct SimulatedSpiNorFlashMedium* Medium;
        /* A count of milliseconds that wraps at 2^32. It is what the bus
         * tells the time by. */
        uint32_t (*Milliseconds)(void);
    };

    /* Powers the part up over that medium, and answers the bus it is on: the
     * same table every time. What the medium holds is what the part holds.
     * There is one part, and powering it up again is a power cycle. The bus's
     * WhileBusy is left NULL for the caller to fill. */
    struct SolidSyslogSpiNorFlashBus* SimulatedSpiNorFlash_PowerUp(const struct SimulatedSpiNorFlashConfig* config);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SIMULATED_SPI_NOR_FLASH_H */
