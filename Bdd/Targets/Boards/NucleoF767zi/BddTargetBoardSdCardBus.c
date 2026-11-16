/* The NUCLEO-F767ZI's MicroSD card bus behind the board seam - see
 * BddTargetBoardSdCardBus.h. The card is on SPI4, which BoardSpi4.c drives. */

#include "BddTargetBoardSdCardBus.h"

#include "Board.h"
#include "BoardSpi4.h"

/* The card driver asks for the start rate at the beginning of every start, so
 * that is where the bus is taken. */
void BddTargetBoardSdCardBus_UseStartRate(void)
{
    BoardSpi4_Take(BOARD_SPI4_RATE_START);
}

void BddTargetBoardSdCardBus_UseRunRate(void)
{
    BoardSpi4_SetRate(BOARD_SPI4_RATE_RUN);
}

void BddTargetBoardSdCardBus_Select(void)
{
    BoardSpi4_Select(BOARD_SPI4_CARD);
}

void BddTargetBoardSdCardBus_Deselect(void)
{
    BoardSpi4_Deselect(BOARD_SPI4_CARD);
}

uint8_t BddTargetBoardSdCardBus_Exchange(uint8_t out)
{
    return BoardSpi4_Exchange(out);
}

uint32_t BddTargetBoardSdCardBus_Milliseconds(void)
{
    return Board_Milliseconds();
}
