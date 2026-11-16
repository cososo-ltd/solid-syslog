/* The NUCLEO-F767ZI's flash bus behind the board seam - see
 * BddTargetBoardFlashBus.h. The W25Q128 is on SPI4, which BoardSpi4.c drives. */

#include "BddTargetBoardFlashBus.h"

#include "Board.h"
#include "BoardSpi4.h"

#include <stddef.h>

enum
{
    IDLE_LINE = 0xFF,
    /* A card needs 74 clocks with its select high before its first command. */
    CARD_WAKE_BYTES = 10,
    CARD_COMMAND_LENGTH = 6,
    /* A card answers a command within eight bytes. */
    CARD_ANSWER_WAIT_BYTES = 9
};

/* CMD0, GO_IDLE_STATE, with its CRC, which a card checks until it is in SPI
 * mode. */
static const uint8_t CARD_GO_IDLE[CARD_COMMAND_LENGTH] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};

static inline void BddTargetBoardFlashBus_PutTheCardInSpiMode(void);

/* The part is run at the rate the card is, on the same wires. */
void BddTargetBoardFlashBus_Take(void)
{
    BoardSpi4_Take(BOARD_SPI4_RATE_START);
    BddTargetBoardFlashBus_PutTheCardInSpiMode();
    BoardSpi4_SetRate(BOARD_SPI4_RATE_RUN);
}

/* The MicroSD card shares these wires, and an image that uses the flash never
 * starts it. A card that has not been started is not yet in SPI mode: it
 * takes the data-out line for its command line whatever its select is doing,
 * and could answer something in the flash's traffic that happened to read as
 * a command to it. Its first command, sent with its select low, puts it in SPI
 * mode, where a high select keeps it off the bus. What it answers is not
 * looked at: with no card fitted there is no answer, and nothing to keep off.
 *
 * It is sent at the rate a card is to be started at, with the flash's select
 * high throughout. */
static inline void BddTargetBoardFlashBus_PutTheCardInSpiMode(void)
{
    for (size_t i = 0U; i < (size_t) CARD_WAKE_BYTES; i++)
    {
        (void) BoardSpi4_Exchange(IDLE_LINE);
    }
    BoardSpi4_Select(BOARD_SPI4_CARD);
    for (size_t i = 0U; i < (size_t) CARD_COMMAND_LENGTH; i++)
    {
        (void) BoardSpi4_Exchange(CARD_GO_IDLE[i]);
    }
    for (size_t i = 0U; i < (size_t) CARD_ANSWER_WAIT_BYTES; i++)
    {
        (void) BoardSpi4_Exchange(IDLE_LINE);
    }
    BoardSpi4_Deselect(BOARD_SPI4_CARD);
    /* A card lets go of its data line a byte after its select rises. */
    (void) BoardSpi4_Exchange(IDLE_LINE);
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
