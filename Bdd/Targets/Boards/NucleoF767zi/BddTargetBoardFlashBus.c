/* The NUCLEO-F767ZI's flash bus behind the board seam - see
 * BddTargetBoardFlashBus.h. The W25Q128 is on SPI4, which BoardSpi4.c drives. */

#include "BddTargetBoardFlashBus.h"

#include "Board.h"
#include "BoardSpi4.h"

/* The part is run at the rate the card is, on the same wires. */
void BddTargetBoardFlashBus_Take(void)
{
    BoardSpi4_Take(BOARD_SPI4_RATE_RUN);
}

void BddTargetBoardFlashBus_Select(void)
{
    BoardSpi4_Select(BOARD_SPI4_FLASH);
}

void BddTargetBoardFlashBus_Deselect(void)
{
    BoardSpi4_Deselect(BOARD_SPI4_FLASH);
}

uint8_t BddTargetBoardFlashBus_Exchange(uint8_t out)
{
    return BoardSpi4_Exchange(out);
}

uint32_t BddTargetBoardFlashBus_Milliseconds(void)
{
    return Board_Milliseconds();
}
