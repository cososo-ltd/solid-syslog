#ifndef BOARD_SPI4_H
#define BOARD_SPI4_H

#include <stdint.h>

/* SPI4 on the CN9 Zio connector, which the two storage breakouts share: a
 * MicroSD card and a W25Q128 flash, with a chip select each. README.md beside
 * this file holds the pin plan.
 *
 * SPI mode 0, most significant bit first. An image drives the card or the
 * flash, and the other's select is held high to keep it off the bus. */

enum BoardSpi4Device
{
    BOARD_SPI4_CARD,
    BOARD_SPI4_FLASH
};

enum BoardSpi4Rate
{
    /* 211 kHz, within the 400 kHz limit for starting a card. */
    BOARD_SPI4_RATE_START,
    /* 6.75 MHz. README.md explains the choice. */
    BOARD_SPI4_RATE_RUN
};

/* Configures the pins, with both selects high, and clocks the bus at `rate`.
 * It can be called again, and reclaims the pins from whatever had them. */
void BoardSpi4_Take(enum BoardSpi4Rate rate);

/* Changes the rate of a bus that has been taken. */
void BoardSpi4_SetRate(enum BoardSpi4Rate rate);

void BoardSpi4_Select(enum BoardSpi4Device device);
void BoardSpi4_Deselect(enum BoardSpi4Device device);

/* Clocks one byte out and returns the byte clocked in with it. A bus with
 * nothing selected on it reads 0xFF. */
uint8_t BoardSpi4_Exchange(uint8_t out);

#endif
