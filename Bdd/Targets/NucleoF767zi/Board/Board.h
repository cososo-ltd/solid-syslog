#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The NUCLEO-F767ZI as firmware sees it: a clock, a console on the ST-LINK
 * virtual COM port, a millisecond count and the three user LEDs. README.md in
 * the directory above holds the pin plan. */

enum BoardLed
{
    BOARD_LED_GREEN,
    BOARD_LED_BLUE,
    BOARD_LED_RED
};

/* Brings the part to 216 MHz from the ST-LINK's 8 MHz clock and opens the
 * console at 115200 8N1. Does not return if the clock will not start: the red
 * LED is lit and the part stays on its internal oscillator, with no console. */
void Board_Init(void);

void Board_ConsoleWrite(const char* text, size_t length);

/* True, with the byte in *received, when one has arrived. Never waits.
 * Bytes are received under interrupt and held until read, 511 of them at
 * most; one that arrives with that many unread is dropped. */
bool Board_ConsoleTryRead(char* received);

/* Milliseconds since Board_Init, wrapping at 2^32. */
uint32_t Board_Milliseconds(void);

void Board_LedToggle(enum BoardLed led);

#endif
