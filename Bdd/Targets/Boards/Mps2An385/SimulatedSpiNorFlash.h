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
     * and all, as it does over the part on a board. */

    /* What the part keeps its bytes in, which is the caller's: memory, or a
     * file. It holds the complement of what the part reads as, so one that
     * has never been written and holds zeros is an erased part. Every span it
     * is asked for is within the part. */
    struct SimulatedSpiNorFlashMedium
    {
        /* False if it could not give them all. */
        bool (*Read)(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, void* buffer, size_t count);
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
