#ifndef BOARD_SPI4_FLASH_H
#define BOARD_SPI4_FLASH_H

/* SPI4 made ready for the W25Q128, whichever flash driver is to use it.
 *
 * The MicroSD card shares the wires, and an image that uses the flash never
 * starts it. A card that has not been started is not yet in SPI mode: it
 * treats MOSI as its command line whatever its select is doing, and could
 * answer flash traffic that happened to read as a command. So the card is
 * put in SPI mode first, where a high select keeps it off the bus. */

/* Takes the bus, keeps the card off it, and leaves it at the rate the board
 * runs the flash at, with both selects high. It can be called again. */
void BoardSpi4Flash_Take(void);

#endif
