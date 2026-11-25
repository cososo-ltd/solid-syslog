#ifndef BDD_TARGET_BOARD_SPI_NOR_FLASH_BUS_H
#define BDD_TARGET_BOARD_SPI_NOR_FLASH_BUS_H

#include "SolidSyslogExternC.h"

struct SolidSyslogSpiNorFlashBus;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The SPI bus the board's NOR flash is on, for a SolidSyslogSpiNorFlash.
     * Part of the board seam - see BddTargetBoard.h - and defined in the
     * directory of a board that has a flash, or simulates one.
     *
     * Makes the bus ready, with anything else on its wires kept off it, and
     * answers the table of bus functions SolidSyslogSpiNorFlashBus.h
     * describes. The same table every time, and it can be called again. */
    struct SolidSyslogSpiNorFlashBus* BddTargetBoardSpiNorFlashBus_Take(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_SPI_NOR_FLASH_BUS_H */
