/* SPI4 made ready for the W25Q128 - see BoardSpi4Flash.h. */

#include "BoardSpi4Flash.h"

#include "BoardSpi4.h"

#include <stddef.h>
#include <stdint.h>

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

static inline void BoardSpi4Flash_PutTheCardInSpiMode(void);

/* The flash runs at the card's rate, on the same wires. */
void BoardSpi4Flash_Take(void)
{
    BoardSpi4_Take(BOARD_SPI4_RATE_START);
    BoardSpi4Flash_PutTheCardInSpiMode();
    BoardSpi4_SetRate(BOARD_SPI4_RATE_RUN);
}

/* CMD0, sent with the card's select low, puts it in SPI mode. The answer is
 * ignored: with no card fitted there is none, and nothing to keep off the
 * bus.
 *
 * It is sent at the card's start rate, with the flash's select high
 * throughout. */
static inline void BoardSpi4Flash_PutTheCardInSpiMode(void)
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
    /* A card releases its data line a byte after its select rises. */
    (void) BoardSpi4_Exchange(IDLE_LINE);
}
