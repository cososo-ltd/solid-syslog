/* The NUCLEO-F767ZI's flash bus, as the bring-up image's flash driver takes
 * it - see BddTargetBoardFlashBus.h. The W25Q128 is on SPI4, which BoardSpi4.c
 * drives. */

#include "BddTargetBoardFlashBus.h"

#include "Board.h"
#include "BoardSpi4.h"
#include "BoardSpi4Flash.h"

void BddTargetBoardFlashBus_Take(void)
{
    BoardSpi4Flash_Take();
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
